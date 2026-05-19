#include "p2p_chat.h"
#include <toml.hpp>



void P2PChatStream::on_read_msg_s(const std::shared_ptr<test::ChatMessage>& m)
{
    on_received_message.emit(_peer_name, m->msg());
}

bool P2PChatStream::send_msg(const std::string& msg)
{
    test::ChatMessage m;
    m.set_msg(msg);
    return send<ChatMessagePair>(m);
}



