#include "p2p_chat.h"
#include <toml.hpp>



mqas::core::StreamVariantErrcode P2PChatStream::on_local_change_msg_s(const std::shared_ptr<test::ReqDirectChat>& msg,
    std::vector<uint8_t>& ret_buf)
{
    auto name = get_name();
    if (name.empty())
        return mqas::core::StreamVariantErrcode::failed;
    msg->set_name(std::move(name));
    mqas::core::ProtoBufMsg::write_msg<ReqDirectChatPair>(ret_buf, *msg);
    return mqas::core::StreamVariantErrcode::ok;
}

mqas::core::StreamVariantErrcode P2PChatStream::on_change_msg_s(const std::shared_ptr<test::ReqDirectChat>& msg,
    std::vector<uint8_t>& ret_buf)
{
    auto name = get_name();
    if (name.empty())
        return mqas::core::StreamVariantErrcode::failed;
    _peer_name = std::move(msg->name());
    on_connected.emit(_peer_name);
    test::RespondDirectChat m;
    m.set_name(std::move(name));
    mqas::core::ProtoBufMsg::write_msg<RespondDirectChatPair>(ret_buf, m);
    return mqas::core::StreamVariantErrcode::ok;
}

void P2PChatStream::on_peer_change_ret_msg_s(mqas::core::StreamVariantErrcode code, const std::shared_ptr<test::RespondDirectChat>& msg)
{
    if (code == mqas::core::StreamVariantErrcode::ok)
    {
        _peer_name = std::move(msg->name());
        on_connected.emit(_peer_name);
    }
}

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

std::string P2PChatStream::get_name() const
{
    auto engine = connect_cxt_->engine_cxt_->engine.lock();
    return toml::find<std::string>(*engine->get_config(), "p2p", "name");
}



