#include <mqas/tools/stream/relay_stream_client.h>
#include <mqas/io/ip.h>
#include <mqas/comm/uuid.h>

namespace mqas::tools { 
    core::StreamVariantErrcode RelayStreamClient::on_local_change_msg_s(const std::shared_ptr<proto::relay::ReqRelay>& req,
        std::vector<uint8_t> &ret_buf)
    {
        auto token = mqas::comm::uuid::to_uuid(req->token().data());
        if(!token)
        {
            LOG(ERROR) << "Relay Bad token!!!";
            return core::StreamVariantErrcode::failed;
        }else
            LOG(ERROR) << "Relay try connect token = " << mqas::comm::uuid::to_high_64(*token);

        return core::StreamVariantErrcode::not_support;
    }

    void RelayStreamClient::on_peer_change_ret_msg_s(mqas::core::StreamVariantErrcode code,
        const std::shared_ptr<proto::relay::RespondRelay>& msg)
    {
        if (code == core::StreamVariantErrcode::ok) 
        {
            _id = msg->id();

            auto conn = connect.lock();
            auto min_size = try_load_datagram_min_size();
            if(min_size)
                conn->set_min_datagram_size(min_size.value());
            io::Ip::str2addr(msg->peer_addr().ip().c_str(),msg->peer_addr().port(),_peer_addr);

            _on_recv_datagram_conn = conn->on_recv_datagram.connect(sigc::mem_fun(*this,&RelayStreamClient::on_recv_datagram));

            proto::relay::ReqReady ready_msg;
            send<tools::relay::ReqReadyPair>(ready_msg);
            LOG(DEBUG) << "relay client send ready";
        }
        on_connect_result.emit(code, msg->code());
    }

    void RelayStreamClient::on_peer_change_ret_msg(core::StreamVariantErrcode code, size_t,const std::shared_ptr<google::protobuf::Message>&)
    {
        on_connect_result.emit(code, std::nullopt);
    }

    //same udp socket interface
    void RelayStreamClient::bind(const sockaddr& addr,unsigned int flags)
    {
        _bind_addr = addr;
    }
    void RelayStreamClient::get_sock_addr(sockaddr& addr) const
    {
        memcpy(&addr,&_bind_addr,sizeof(sockaddr));
    }
    int RelayStreamClient::try_send(const std::vector<std::span<uint8_t>>& d, const sockaddr& addr)
    {
        if(!io::Ip::compare_ip(addr,_peer_addr))
        {
            auto expected = io::Ip::addr2str(_peer_addr);
            auto trysend = io::Ip::addr2str(addr);
            LOG(DEBUG) << "relay try addr compare failed expected " 
                << expected << ':' << io::Ip::addr_get_port(_peer_addr) << " send to " << trysend << ':' << io::Ip::addr_get_port(addr);
            return 0;
        }
        if(_id == 0)
            return 0;
        int bytes = 0;
        auto conn = connect.lock();
        for (auto& it : d)
        {
            if(it.size() == 0)
                continue;
            //write_lazy(it);
            if(conn->write_datagram(it))
                bytes += it.size();
        }
        if(!conn->flush_datagram())
            return 0;
        #if !NDEBUG
        LOG(INFO) << "relay try send " << bytes << " bytes";
        #endif
        return bytes;
    }
    int RelayStreamClient::try_send(const std::span<uint8_t>& d, const sockaddr& addr)
    {
        if(!io::Ip::compare_ip(addr,_peer_addr))
        {
            auto expected = io::Ip::addr2str(_peer_addr);
            auto trysend = io::Ip::addr2str(addr);
            LOG(DEBUG) << "relay try addr compare failed expected " 
                << expected << ':' << io::Ip::addr_get_port(_peer_addr) << " send to " << trysend << ':' << io::Ip::addr_get_port(addr);
            return 0;
        }
        if(_id == 0 || d.size() == 0)
            return 0;
        //write_lazy(d);
        auto conn = connect.lock();
        conn->write_datagram(d);
        if(!conn->flush_datagram())
            return 0;
        return d.size();
    }
    void RelayStreamClient::recv_start(){}

    void RelayStreamClient::on_close()
    {
        _id = 0;
        if(_on_recv_datagram_conn)
            _on_recv_datagram_conn.disconnect();
        IStream::on_close();
    }

    void RelayStreamClient::on_recv_datagram(const uint8_t* buf,size_t size)
    {
        std::span<uint8_t> span((uint8_t*)buf,size);
        on_recv_signal.emit(nullptr,span,span.size(),&_peer_addr,0);
    }

    std::optional<uint16_t> RelayStreamClient::try_load_datagram_min_size() const
    {
        auto e = connect_cxt_->engine_cxt_->engine.lock();
        auto size = toml::find_or<int>(*e->get_config(),"relay","datagram_min_size",-1);
        if(size < 0)
            return {};
        else
            return { (uint16_t)size };
    }

    void RelayStreamClient::on_read_msg_s(const std::shared_ptr<proto::relay::RespondReady>& m)
    {
        LOG(DEBUG) << "relay client recvive ready respond";
        on_ready.emit();
    }
}