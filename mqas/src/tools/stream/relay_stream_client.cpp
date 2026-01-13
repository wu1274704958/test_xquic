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
            LOG(INFO) << "Relay try connect token = " << mqas::comm::uuid::to_high_64(*token);

        return core::StreamVariantErrcode::not_support;
    }

    void RelayStreamClient::on_peer_change_ack_msg_s(mqas::core::StreamVariantErrcode code,
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
            io::Ip::str2addr(msg->self_addr().ip().c_str(),msg->self_addr().port(),_bind_addr);

#if !NDEBUG
            LOG(DEBUG) << "relay client peer address = " << msg->peer_addr().ip() << ':' << msg->peer_addr().port();
            LOG(DEBUG) << "relay client self address = " << msg->self_addr().ip() << ':' << msg->self_addr().port();
#endif

            _on_recv_datagram_conn = conn->on_recv_datagram_signal.connect(sigc::mem_fun(*this,&RelayStreamClient::on_recv_datagram));

            proto::relay::ReqReady ready_msg;
            send<tools::relay::ReqReadyPair>(ready_msg);
            LOG(DEBUG) << "relay client send ready";
        }
        on_connect_result.emit(code, msg->code());
    }

    void RelayStreamClient::on_peer_change_ack_msg(core::StreamVariantErrcode code, size_t,const std::shared_ptr<google::protobuf::Message>&)
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
    void RelayStreamClient::get_peer_addr(sockaddr& addr) const
    {
        memcpy(&addr,&_peer_addr,sizeof(sockaddr));
    }
    int RelayStreamClient::try_send(const std::vector<std::span<uint8_t>>& d, const sockaddr& addr)
    {
        // if(!io::Ip::compare_ip(addr,_peer_addr))
        // {
        //     auto expected = io::Ip::addr2str(_peer_addr);
        //     auto trysend = io::Ip::addr2str(addr);
        //     LOG(DEBUG) << "relay try addr compare failed expected " 
        //         << expected << ':' << io::Ip::addr_get_port(_peer_addr) << " send to " << trysend << ':' << io::Ip::addr_get_port(addr);
        //     return 0;
        // }
        if(_id == 0)
            return 0;
        size_t bytes = 0;
        size_t start = 0;
        auto conn = connect.lock();
        for (auto& it : d)
        {
            if(it.size() == 0)
                continue;
            bytes += it.size();
            if(_buffer.size() < bytes)
                _buffer.resize(bytes);
            std::memcpy(_buffer.data() + start,it.data(),it.size());
            start += it.size();
        }
        std::span<uint8_t> span{ _buffer.data(),bytes };
        conn->write_datagram(span);
        if(!conn->flush_datagram())
            return 0;
        #if !NDEBUG
        LOG(INFO) << "relay try send to " << io::Ip::addr2str(addr) << ':' << io::Ip::addr_get_port(addr) <<  " " << bytes << " bytes";
        #endif
        return bytes;
    }
    int RelayStreamClient::try_send(const std::span<uint8_t>& d, const sockaddr& addr)
    {
        // if(!io::Ip::compare_ip(addr,_peer_addr))
        // {
        //     auto expected = io::Ip::addr2str(_peer_addr);
        //     auto trysend = io::Ip::addr2str(addr);
        //     LOG(DEBUG) << "relay try addr compare failed expected " 
        //         << expected << ':' << io::Ip::addr_get_port(_peer_addr) << " send to " << trysend << ':' << io::Ip::addr_get_port(addr);
        //     return 0;
        // }
        if(_id == 0 || d.size() == 0)
            return 0;
        //write_lazy(d);
        auto conn = connect.lock();
        conn->write_datagram(d);
        if(!conn->flush_datagram())
            return 0;
        #if !NDEBUG
        LOG(INFO) << "relay try send to " << io::Ip::addr2str(addr) << ':' << io::Ip::addr_get_port(addr) <<  " " << d.size() << " bytes";
        #endif
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
        
        if(on_recv_signal.size() < _listener_count)
        {
            _recv_buffer.push(span);
            if(!_check_has_listener_task)
            {
                LOG(INFO) << "relay on receive but no listen launch check task";
                _check_has_listener_task = connect_cxt_->engine_cxt_->io_cxt.make_shared<io::Idle>();
                _check_has_listener_task->start(std::bind(&RelayStreamClient::on_check_has_listener,this,std::placeholders::_1));
            }
        }else{
            while(!_recv_buffer.empty())
            {
                auto buf = _recv_buffer.pop();
                emit_msg(*buf);
            }
            emit_msg(span);
        }
    }

    void RelayStreamClient::on_check_has_listener(io::Idle* idle)
    {
        if(on_recv_signal.size() >= _listener_count)
        {
            idle->stop();
            while(!_recv_buffer.empty())
            {
                auto buf = _recv_buffer.pop();
                LOG(INFO) << "relay listener in place emit msg " << buf->size() << " bytes " << io::Ip::addr2str(_peer_addr) << ":" <<  io::Ip::addr_get_port(_peer_addr);
                emit_msg(*buf);
            }
        }
    }

    void RelayStreamClient::emit_msg(const std::span<uint8_t>& buf) const
    {
        #if !NDEBUG
        LOG(INFO) << "relay on receive " << buf.size() << " bytes " << io::Ip::addr2str(_peer_addr) << ":" <<  io::Ip::addr_get_port(_peer_addr);
        #endif
        on_recv_signal.emit(nullptr,buf,buf.size(),&_peer_addr,0);
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