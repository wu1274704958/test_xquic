#pragma once

#include <mqas/macro.h>
#include <mqas/tools/stream/MsgDef.h>
#include <mqas/core/pb_stream.h>
#include <sigc++/sigc++.h>

namespace mqas::tools{

class MQAS_EXTERN RelayStreamClient : public core::ProtoBufStream<RelayStreamClient,relay::ReqRelayPair,relay::RespondRelayPair> 
{
public://udp same
RelayStreamClient(){}
void bind(const sockaddr& addr,unsigned int flags);
void get_sock_addr(sockaddr& addr) const;
int try_send(const std::vector<std::span<uint8_t>>& d, const sockaddr& addr);
int try_send(const std::span<uint8_t>& d, const sockaddr& addr);
void recv_start();

public:
sigc::signal<void(io::UdpSocket*, const std::optional<std::span<uint8_t>>&, ssize_t nread, const sockaddr*, unsigned)> on_recv_signal;
sigc::signal<void(core::StreamVariantErrcode,std::optional<proto::relay::RespondRelay_Code>)> on_connect_result;
public:
core::StreamVariantErrcode on_local_change_msg_s(const std::shared_ptr<proto::relay::ReqRelay>& req,std::vector<uint8_t> &ret_buf);
void on_peer_change_ret_msg_s(mqas::core::StreamVariantErrcode code,const std::shared_ptr<proto::relay::RespondRelay>& msg);
void on_peer_change_ret_msg(core::StreamVariantErrcode code, size_t,const std::shared_ptr<google::protobuf::Message>&);
size_t on_read(const std::span<const uint8_t>& buffer);
void on_close();
protected:
void on_recv_datagram(const uint8_t* buf,size_t size);

private:
    uint32_t _id;
    ::sockaddr _peer_addr;
    ::sockaddr _bind_addr;
    sigc::connection _on_recv_datagram_conn;
};

}