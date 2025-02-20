#include <mqas/tools/stream/relay_stream.h>
#include <mqas/comm/locator.h>
#include <mqas/tools/model/relay_model.h>
#include <mqas/io/ip.h>

namespace mqas::tools{
    core::StreamVariantErrcode RelayStream::on_change_msg_s(const std::shared_ptr<proto::relay::ReqRelay>& req,
        std::vector<uint8_t> &ret_buf)
    {
        auto model = comm::locator::inst()->get<relay::relay_model>();
        auto conn = connect.lock();
        if(!model || ! conn)
            return core::StreamVariantErrcode::failed;
        auto data = model.value();
        const ::sockaddr *local,*peer;
        if(!conn->get_sockaddr(&local,&peer))
            return core::StreamVariantErrcode::failed;  
        _addr = *peer;
        proto::relay::RespondRelay respond;
        respond.set_id(0);

#if !NDEBUG
        LOG(INFO) << "relay req " << io::Ip::addr2str(_addr) << ':' << io::Ip::addr_get_port(_addr) << 
        " connect " <<  req->address().ip().c_str() << ':' << req->address().port();
#endif

        if(!io::Ip::str2addr(req->address().ip().c_str(),req->address().port(),_connect_addr))
        {
            respond.set_code(proto::relay::RespondRelay_Code::RespondRelay_Code_bad_arguments);
            core::ProtoBufMsg::write_msg<tools::relay::RespondRelayPair>(ret_buf,respond);
            return core::StreamVariantErrcode::failed;
        }
    
        auto [state,id] = data.get().try_connect(*peer,_connect_addr,this->weak_from_this());
       
        switch (state)
        {
        case relay::RelayState::invaild:
        case relay::RelayState::mismatching:
            return core::StreamVariantErrcode::failed;
        case relay::RelayState::relaying:
            _id = id;
            {
                auto ptr = data.get().find_other_peer_stream<RelayStream>(id,_connect_addr);
                if(ptr == nullptr)
                    return core::StreamVariantErrcode::failed;
                _other_peer = ptr;
                ptr->send_respond(id,proto::relay::RespondRelay_Code::RespondRelay_Code_success,true,
                    std::dynamic_pointer_cast<RelayStream>(this->shared_from_this()));
                reg_on_recv_datagram();
            }
            respond.set_id(_id);
            respond.set_code(proto::relay::RespondRelay_Code::RespondRelay_Code_success);
            core::ProtoBufMsg::write_msg<tools::relay::RespondRelayPair>(ret_buf,respond);
            return core::StreamVariantErrcode::ok;
        case relay::RelayState::waiting:
            respond.set_code(proto::relay::RespondRelay_Code::RespondRelay_Code_waiting_peer);
            core::ProtoBufMsg::write_msg<tools::relay::RespondRelayPair>(ret_buf,respond);
            launch_timeout_timer();
            return core::StreamVariantErrcode::skip_and_manual;
        }
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
        }
        respond.set_id(_id);
        respond.set_code(code);

        send_sv_msg<tools::relay::RespondRelayPair,core::stream_variant_cmd::req_use_stream_tag>(respond,stream_tag_,
            0,1, code == proto::relay::RespondRelay_Code::RespondRelay_Code_success ? core::StreamVariantErrcode::ok : core::StreamVariantErrcode::failed,lazy);
    }

    size_t RelayStream::on_read(const std::span<const uint8_t>& buf)
    {
        return 0;
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
            model.value().get().remove_waiting(_addr,_connect_addr);
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
                model->get().remove_waiting(_addr,_connect_addr);
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
        _on_recv_datagram_conn = conn->on_recv_datagram.connect(sigc::mem_fun(*this,&RelayStream::on_recv_datagram));
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
}