#include <mqas/tools/stream/relay_stream_client.h>
#include <mqas/io/ip.h>

namespace mqas::tools { 
    core::StreamVariantErrcode RelayStreamClient::on_local_change_msg_s(const std::shared_ptr<proto::relay::ReqRelay>& req,
        std::vector<uint8_t> &ret_buf)
    {
        if(!io::Ip::str2addr(req->address().ip().c_str(),req->address().port(),_peer_addr))
            return core::StreamVariantErrcode::failed;
        return core::StreamVariantErrcode::not_support;
    }

    void RelayStreamClient::on_peer_change_ret_msg_s(mqas::core::StreamVariantErrcode code,
        const std::shared_ptr<proto::relay::RespondRelay>& msg)
    {
        if (code == core::StreamVariantErrcode::ok) 
        {
            _id = msg->id();
        }
        on_connect_result.emit(code, msg->code());
    }

    void RelayStreamClient::on_peer_change_ret_msg(core::StreamVariantErrcode code, size_t,const std::shared_ptr<google::protobuf::Message>&)
    {
        on_connect_result.emit(code, std::nullopt);
    }

    size_t RelayStreamClient::on_read(const std::span<const uint8_t>& buffer)
    {
        on_recv_signal.emit(nullptr,*reinterpret_cast<const std::span<uint8_t>*>(&buffer),buffer.size(),&_peer_addr,0);
        return buffer.size();
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
            return 0;
        int bytes = 0;
        for (auto& it : d)
        {
            if(it.size() == 0)
                continue;
            write_lazy(it);
            bytes += d.size();
        }
        return bytes;
    }
    int RelayStreamClient::try_send(const std::span<uint8_t>& d, const sockaddr& addr)
    {
        if(!io::Ip::compare_ip(addr,_peer_addr) || d.size() == 0)
            return 0;
        write_lazy(d);
        return d.size();
    }
    void RelayStreamClient::recv_start(){}
}