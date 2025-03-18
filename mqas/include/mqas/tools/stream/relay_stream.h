#pragma once

#include <mqas/macro.h>
#include <mqas/core/pb_stream.h>
#include "MsgDef.h"
#include <mqas/io/timer.h>
#include <boost/uuid/uuid.hpp>

namespace mqas::tools {

class MQAS_EXTERN RelayStream : public core::ProtoBufStream<RelayStream,
    tools::relay::ReqRelayPair,tools::relay::RespondRelayPair,
    tools::relay::ReqReadyPair,tools::relay::RespondReadyPair>
{
    public:
    core::StreamVariantErrcode on_change_msg_s(const std::shared_ptr<proto::relay::ReqRelay>& req,std::vector<uint8_t> &ret_buf);
    void on_close();
    void on_read_msg_s(const std::shared_ptr<proto::relay::ReqReady>& m);
    private:
    void send_respond(uint32_t id,proto::relay::RespondRelay_Code code,bool lazy,std::weak_ptr<RelayStream> other_peer);
    void launch_timeout_timer();
    void stop_timeout_timer();
    void on_timeout(io::Timer* t);
    void reg_on_recv_datagram();
    void on_recv_datagram(const uint8_t* buf,size_t size);
    std::optional<uint16_t> try_load_datagram_min_size() const;
    void set_peer_addr(mqas::tools::proto::relay::Address* addr);
    void set_self_addr(mqas::tools::proto::relay::Address* addr);

    private:
    uint32_t _id = 0;
    ::sockaddr _addr;
    boost::uuids::uuid _token;
    std::weak_ptr<RelayStream> _other_peer;
    std::shared_ptr<io::Timer> _timeout_timer;
    sigc::connection _on_recv_datagram_conn;
    bool _ready:1 = false;
};

}