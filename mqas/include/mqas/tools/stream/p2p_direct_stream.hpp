#pragma once
#include <easylogging++.h>
#include <mqas/comm/locator.h>
#include <mqas/comm/uuid.h>
#include <mqas/core/protobuf_msg.h>

namespace mqas::tools::p2p_direct {

template<typename S,typename... MSG>
mqas::core::StreamVariantErrcode P2PDirectStream<S,MSG...>::on_local_change_msg_s(
    const std::shared_ptr<proto::p2p_client::RequestDirectConnect>& msg,
    std::vector<uint8_t>& ret_buf)
{
    if (msg->name().empty())
    {
        auto name = get_name();
        if (name.empty())
            return mqas::core::StreamVariantErrcode::failed;
        msg->set_name(std::move(name));
    }
    if (!msg->has_token())
        return mqas::core::StreamVariantErrcode::failed;
    mqas::core::ProtoBufMsg::write_msg<ReqDirectConnectPair>(ret_buf, *msg);
    return mqas::core::StreamVariantErrcode::ok;
}

template<typename S,typename... MSG>
const std::string& P2PDirectStream<S,MSG...>::get_name()
{
    if (!_name.empty())
        return _name;
    const auto engine = this->connect_cxt_->engine_cxt_->engine.lock();
    _name = toml::find<std::string>(*engine->get_config(), "p2p", "name");
    return _name;
}

template<typename S,typename... MSG>
void P2PDirectStream<S,MSG...>::on_connected(const std::string& peer_name)
{
    _connected = true;
    on_connected_signal.emit(peer_name);
}

template<typename S,typename... MSG>
void P2PDirectStream<S,MSG...>::on_disconnected(const std::string& reason)
{
    _connected = false;
    on_disconnected_signal.emit(reason);
}

template<typename S,typename... MSG>
void P2PDirectStream<S,MSG...>::on_connect_failed(proto::p2p_client::RetCode code)
{
    _connected = false;
    on_connect_failed_signal.emit(code);
}

template<typename S,typename... MSG>
bool P2PDirectStream<S,MSG...>::req_quit(const std::string& reason, bool lazy)
{
    if (!_connected)
        return false;
    proto::p2p_client::RequestDirectQuit req;
    if (!reason.empty())
        req.set_reason(reason);
    return this->template send_req_quit<ReqDirectQuitPair>(req, lazy);
}

// ── server side: handle incoming RequestDirectConnect ────────────────────────

template<typename S,typename... MSG>
core::StreamVariantErrcode P2PDirectStream<S,MSG...>::on_change_msg_s(
    const std::shared_ptr<proto::p2p_client::RequestDirectConnect>& req,
    std::vector<uint8_t>& ret_buf)
{
    LOG(INFO) << "[P2PDirectStream] recv RequestDirectConnect from '" << req->name() << "'";
    proto::p2p_client::RespondDirectConnect resp;

#define RESP_CODE(C)                                                                        \
    {                                                                                       \
        resp.set_code(C);                                                                   \
        core::ProtoBufMsg::write_msg<RespondDirectConnectPair>(ret_buf, resp);              \
        on_connect_failed(C);                                                               \
        return core::StreamVariantErrcode::failed;                                          \
    }

    if (!req->has_token())
        RESP_CODE(proto::p2p_client::not_submit_token)

    if (this->connect.expired())
        RESP_CODE(proto::p2p_client::unknown_error)

    const auto result = comm::locator::inst()->get<std::shared_ptr<mqas::tools::proto::p2p::NotifyConnectResult>>(this->connect.lock());
    if (!result || !result.value().get()->has_verify_token())
        RESP_CODE(proto::p2p_client::token_not_found)

    const auto self_token = mqas::comm::uuid::to_uuid(result.value().get()->verify_token().data());
    const auto token = mqas::comm::uuid::to_uuid(req->token().data());

    LOG(INFO) << "peer submit token size:" << req->token().data().size() << " value:" << (token ? mqas::comm::uuid::to_high_64(*token) : 0);

    if (token != self_token)
    {
        LOG(DEBUG) << "token mismatch, self token size:" << result.value().get()->verify_token().data().size()
                   << " value:" << (self_token ? mqas::comm::uuid::to_high_64(*self_token) : 0);
        RESP_CODE(proto::p2p_client::wrong_token)
    }

    bool accepted = true;
    if (!on_connect_request_signal.empty())
        accepted = on_connect_request_signal.emit(req);

    if (!accepted)
        RESP_CODE(proto::p2p_client::refused_by_peer)

    resp.set_code(proto::p2p_client::ok);
    resp.set_name(get_name());
    core::ProtoBufMsg::write_msg<RespondDirectConnectPair>(ret_buf, resp);
    _peer_name = req->name();

    on_connected(_peer_name);
    return core::StreamVariantErrcode::ok;
#undef RESP_CODE
}

// ── client side: ack for RequestDirectConnect ────────────────────────────────

template<typename S,typename... MSG>
void P2PDirectStream<S,MSG...>::on_peer_change_ack_msg_s(core::StreamVariantErrcode code,
    const std::shared_ptr<proto::p2p_client::RespondDirectConnect>& msg)
{
    LOG(INFO) << "[P2PDirectStream] on_connect_result code=" << static_cast<int>(code)
              << " ret=" << static_cast<int>(msg->code());

    if (code == core::StreamVariantErrcode::ok)
    {
        _peer_name = msg->name();
        on_connected(_peer_name);
    }
    else
    {
        on_connect_failed(msg->code());
    }
}

template<typename S,typename... MSG>
core::StreamVariantErrcode P2PDirectStream<S,MSG...>::on_peer_quit_msg_s(
    const std::shared_ptr<proto::p2p_client::RequestDirectQuit>& req,
    std::vector<uint8_t>& buf)
{
    on_disconnected(req->reason());
    proto::p2p_client::RespondDirectQuit resp;
    resp.set_code(proto::p2p_client::RetCode::ok);
    core::ProtoBufMsg::write_msg<RespondDirectQuitPair>(buf, resp);
    _connected = false;
    return core::StreamVariantErrcode::ok;
}

template<typename S,typename... MSG>
void P2PDirectStream<S,MSG...>::on_peer_quit_ack_msg_s(
    core::StreamVariantErrcode e,
    const std::shared_ptr<proto::p2p_client::RespondDirectQuit>& ack)
{
    LOG(INFO) << "[P2PDirectStream] on_peer_quit_ack_msg_s code=" << static_cast<int>(e)
              << " ret=" << static_cast<int>(ack->code());
    if (e == core::StreamVariantErrcode::ok)
    {
        _connected = false;
        on_disconnected("Proactively request");
    }
}

} // namespace mqas::tools::p2p_direct
