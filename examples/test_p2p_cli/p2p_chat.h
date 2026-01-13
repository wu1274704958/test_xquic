#pragma once

#include <mqas/core/pb_stream.h>
#include "test_p2p.pb.h"
#include <sigc++/sigc++.h>

using ReqDirectChatPair = mqas::core::PBMsgPair<1,test::ReqDirectChat>;
using RespondDirectChatPair = mqas::core::PBMsgPair<2, test::RespondDirectChat>;
using ChatMessagePair = mqas::core::PBMsgPair<3, test::ChatMessage>;

class P2PChatStream : public mqas::core::ProtoBufStream<P2PChatStream,
	ReqDirectChatPair, RespondDirectChatPair, ChatMessagePair
>
{
public:
    sigc::signal<void(const std::string&, const std::string&)> on_received_message;
    sigc::signal<void(const std::string&)> on_connected;
    mqas::core::StreamVariantErrcode on_change_msg_s(const std::shared_ptr<test::ReqDirectChat>& msg,
        std::vector<uint8_t>& ret_buf);
    mqas::core::StreamVariantErrcode on_local_change_msg_s(const std::shared_ptr<test::ReqDirectChat>& msg,
        std::vector<uint8_t>& ret_buf);
    void on_peer_change_ack_msg_s(mqas::core::StreamVariantErrcode code, const std::shared_ptr<test::RespondDirectChat>& msg);

    void on_read_msg_s(const std::shared_ptr<test::ChatMessage>& m);

    bool send_msg(const std::string& msg);

    std::string get_name() const;

private:
    std::string _peer_name;
};

