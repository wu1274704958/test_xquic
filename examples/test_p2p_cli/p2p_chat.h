#pragma once

#include <mqas/tools/stream/p2p_direct_stream.h>
#include "test_p2p.pb.h"
#include <sigc++/sigc++.h>

using ChatMessagePair = mqas::core::PBMsgPair<5, test::ChatMessage>;

class P2PChatStream : public mqas::tools::p2p_direct::P2PDirectStream<P2PChatStream,ChatMessagePair>
{
public:
    sigc::signal<void(const std::string&, const std::string&)> on_received_message;

    void on_read_msg_s(const std::shared_ptr<test::ChatMessage>& m);
    bool send_msg(const std::string& msg);
};

