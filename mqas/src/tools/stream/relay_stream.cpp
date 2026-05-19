#include <mqas/tools/stream/relay_stream.h>
#include <mqas/comm/locator.h>
#include <mqas/tools/model/relay_model.h>
#include <mqas/io/ip.h>
#include <mqas/comm/uuid.h>

namespace mqas::tools{
    core::StreamVariantErrcode RelayStream::on_change_msg_s(const std::shared_ptr<proto::relay::ReqRelay>& req,
        std::vector<uint8_t> &ret_buf)
    {
        auto model = comm::locator::inst()->get<relay::relay_model>();
        auto conn = connect.lock();
        auto token = mqas::comm::uuid::to_uuid(req->token().data());
        if(!model || ! conn || !token)
            return core::StreamVariantErrcode::failed;
        auto data = model.value();
        const ::sockaddr *local,*peer;
        if(!conn->get_sockaddr(&local,&peer))
            return core::StreamVariantErrcode::failed;  
        _addr = *peer;
        _token = *token;
        proto::relay::RespondRelay respond;
        respond.set_id(0);

        LOG(INFO) << "relay req " << io::Ip::addr2str(_addr) << ':' << io::Ip::addr_get_port(_addr) << 
        " connect " <<  mqas::comm::uuid::to_high_64(*token);
    
        auto [state,id] = data.get().try_connect(*peer,*token,this->weak_from_this());
       
        switch (state)
        {
        case relay::RelayState::invaild:
        case relay::RelayState::mismatching:
            return core::StreamVariantErrcode::failed;
        case relay::RelayState::relaying:
            _id = id;
            {
                auto ptr = data.get().find_other_peer_stream<RelayStream>(id,this->shared_from_this());
                if(ptr == nullptr)
                    return core::StreamVariantErrcode::failed;
                _other_peer = ptr;
                ptr->send_respond(id,proto::relay::RespondRelay_Code::RespondRelay_Code_success,true,
                    std::dynamic_pointer_cast<RelayStream>(this->shared_from_this()));
                reg_on_recv_datagram();
            }
            respond.set_id(_id);
            respond.set_code(proto::relay::RespondRelay_Code::RespondRelay_Code_success);
            set_peer_addr(respond.mutable_peer_addr());
            set_self_addr(respond.mutable_self_addr());
            core::ProtoBufMsg::write_msg<tools::relay::RespondRelayPair>(ret_buf,respond);
            return core::StreamVariantErrcode::ok;
        case relay::RelayState::waiting:
            respond.set_code(proto::relay::RespondRelay_Code::RespondRelay_Code_waiting_peer);
            core::ProtoBufMsg::write_msg<tools::relay::RespondRelayPair>(ret_buf,respond);
            launch_timeout_timer();
            return core::StreamVariantErrcode::skip_and_manual;
        }
        return core::StreamVariantErrcode::failed;
    }

    void RelayStream::set_peer_addr(mqas::tools::proto::common::Address* addr)
    {
        auto peer = _other_peer.lock();
        addr->set_ip(io::Ip::addr2str(peer->_addr));
        addr->set_port(io::Ip::addr_get_port(peer->_addr));
    }

    void RelayStream::set_self_addr(mqas::tools::proto::common::Address* addr)
    {
        addr->set_ip(io::Ip::addr2str(_addr));
        addr->set_port(io::Ip::addr_get_port(_addr));
    }

    void RelayStream::send_respond(uint32_t id,proto::relay::RespondRelay_Code code,bool lazy,std::weak_ptr<RelayStream> other_peer)
    {
        proto::relay::RespondRelay respond;
        if(code == proto::relay::RespondRelay_Code::RespondRelay_Code_success)
        {
            _id = id;
            _other_peer = other_peer;
            stop_timeout_timer();
            reg_on_recv_datagram();
            set_peer_addr(respond.mutable_peer_addr());
            set_self_addr(respond.mutable_self_addr());
        }
        respond.set_id(_id);
        respond.set_code(code);

        send_sv_msg<tools::relay::RespondRelayPair,core::stream_variant_cmd::req_use_stream_tag>(respond,_stream_tag,
            0,1, code == proto::relay::RespondRelay_Code::RespondRelay_Code_success ? core::StreamVariantErrcode::ok : core::StreamVariantErrcode::failed,lazy);
    }

    void RelayStream::on_read_msg_s(const std::shared_ptr<proto::relay::ReqReady>& m)
    {
        auto model = comm::locator::inst()->get<relay::relay_model>();
        if(_ready || !model || _id <= 0)
            return;
            
        _ready = true;
        auto ptr = _other_peer.lock();
        if(ptr && ptr->_ready)
        {
            proto::relay::RespondReady msg;
            ptr->send_lazy<tools::relay::RespondReadyPair>(msg);
            send<tools::relay::RespondReadyPair>(msg);
            LOG(DEBUG) << "relay server send ready respond";
        }
    }

    void RelayStream::launch_timeout_timer()
    {
        if(_timeout_timer)
            return;
        _timeout_timer = connect_cxt_->engine_cxt_->io_cxt.make_shared<io::Timer>();
        _timeout_timer->start(std::bind(&RelayStream::on_timeout,this,std::placeholders::_1),1000 * 30,0);
    }
    void RelayStream::stop_timeout_timer()
    {
        if(!_timeout_timer)
            return;
        _timeout_timer->stop();
        _timeout_timer.reset();
    }
    void RelayStream::on_timeout(io::Timer* t)
    {
        auto model = comm::locator::inst()->get<relay::relay_model>();
        if(model)
        {
            model.value().get().remove_waiting(_addr,_token);
        }
        close();
    }

    void RelayStream::on_close() 
    {
        auto model = comm::locator::inst()->get<relay::relay_model>();
        if(model)
        {
            if(_id > 0)
                model->get().remove_relay(_id);
            else
                model->get().remove_waiting(_addr,_token);
        }
        auto ptr = _other_peer.lock();
        if(ptr)
            ptr->close();
        if(_on_recv_datagram_conn)
            _on_recv_datagram_conn.disconnect();
        IStream::on_close();
    }

    void RelayStream::reg_on_recv_datagram()
    {
        if(_on_recv_datagram_conn)
            return;
        auto conn = connect.lock();
        _on_recv_datagram_conn = conn->on_recv_datagram_signal.connect(sigc::mem_fun(*this,&RelayStream::on_recv_datagram));
        auto min_size = try_load_datagram_min_size();
        if(min_size)
            conn->set_min_datagram_size(min_size.value());
    }

    void RelayStream::on_recv_datagram(const uint8_t* buf,size_t size)
    {
        std::span<uint8_t> span((uint8_t*)buf,size);
        auto ptr = _other_peer.lock();
        if(ptr)
        {
            auto conn = ptr->connect.lock();
            conn->write_datagram(span);
            if(!conn->flush_datagram())
                close();
        }else
            close();
    }

    std::optional<uint16_t> RelayStream::try_load_datagram_min_size() const
    {
        auto e = connect_cxt_->engine_cxt_->engine.lock();
        auto size = toml::find_or<int>(*e->get_config(),"relay","datagram_min_size",-1);
        if(size < 0)
            return {};
        else
            return { (uint16_t)size };
    }
}