#pragma once

#include <mqas/macro.h>
#include <mqas/core/pb_stream.h>
#include "MsgDef.h"
#include <mqas/io/timer.h>

namespace mqas::tools {

class MQAS_EXTERN RelayStream : public core::ProtoBufStream<RelayStream,tools::relay::ReqRelayPair,tools::relay::RespondRelayPair>
{
    public:
    core::StreamVariantErrcode on_change_msg_s(const std::shared_ptr<proto::relay::ReqRelay>& req,std::vector<uint8_t> &ret_buf);
    void on_close();
    size_t on_read(const std::span<const uint8_t>& buf);
    private:
    void send_respond(uint32_t id,proto::relay::RespondRelay_Code code,bool lazy,std::weak_ptr<RelayStream> other_peer);
    void launch_timeout_timer();
    void stop_timeout_timer();
    void on_timeout(io::Timer* t);
    void reg_on_recv_datagram();
    void on_recv_datagram(const uint8_t* buf,size_t size);

    private:
    uint32_t _id = 0;
    ::sockaddr _addr;
    ::sockaddr _connect_addr;
    std::weak_ptr<RelayStream> _other_peer;
    std::shared_ptr<io::Timer> _timeout_timer;
    sigc::connection _on_recv_datagram_conn;
};

}