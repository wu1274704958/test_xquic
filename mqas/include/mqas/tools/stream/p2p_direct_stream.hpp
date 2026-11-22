#pragma once
#include <easylogging++.h>
#include <mqas/comm/locator.h>
#include <mqas/comm/uuid.h>
#include <mqas/core/protobuf_msg.h>
#include <openssl/crypto.h>

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
    auto connect = this->connect.lock();
    if (!connect)
        return mqas::core::StreamVariantErrcode::failed;
    const auto security = connect->security_info();
    if (security.handshake != core::HandshakeState::Succeeded ||
        security.authentication != core::PeerAuthentication::ExternalPsk)
        return mqas::core::StreamVariantErrcode::failed;
    auto self_id = comm::locator::inst()->get<LocalPeerId>(connect);

    if (!self_id)
        return mqas::core::StreamVariantErrcode::failed;

    _self_id = self_id.value();
    auto locator = comm::locator::inst();
    auto registry = locator->get<std::shared_ptr<DirectStreamRegistry>>(connect);
    if (!registry) {
        locator->deposit_cxt<std::shared_ptr<DirectStreamRegistry>>(connect, std::make_shared<DirectStreamRegistry>());
        registry = locator->get<std::shared_ptr<DirectStreamRegistry>>(connect);
    }
    const auto& connection_registry = registry->get();
    _local_stream_id = msg->stream_id();
    _entry = connection_registry->reserve_local(this->get_origin(), _local_stream_id);
    if (!_entry) return core::StreamVariantErrcode::failed;
    msg->set_peer_id(_self_id);
    msg->clear_token();

    mqas::core::ProtoBufMsg::write_msg<ReqDirectConnectPair>(ret_buf, *msg);
    _awaiting_direct_ack = true;
    return mqas::core::StreamVariantErrcode::ok;
}

template<typename S,typename... MSG>
const std::string& P2PDirectStream<S,MSG...>::get_name()
{
    if (!_name.empty())
        return _name;
    const auto engine = this->connect_cxt_->engine_cxt_->engine.lock();
    const auto config = this->connect_cxt_->config ? this->connect_cxt_->config : engine->get_config();
    _name = toml::find<std::string>(*config, "p2p", "name");
    return _name;
}

template<typename S,typename... MSG>
void P2PDirectStream<S,MSG...>::on_connected(const std::string& peer_name)
{
    _connected = true;
    on_connected_signal.emit(peer_name,_peer_id);
}

template<typename S,typename... MSG>
void P2PDirectStream<S,MSG...>::on_disconnected(const std::string& reason)
{
    _connected = false;
    release_stream_id();
    on_disconnected_signal.emit(reason,_peer_id);
}

template<typename S,typename... MSG>
void P2PDirectStream<S,MSG...>::on_connect_failed(proto::p2p_client::RetCode code)
{
    _connected = false;
    on_connect_failed_signal.emit(code,_peer_id);
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
        release_stream_id();                                                                \
        on_connect_failed(C);                                                               \
        return core::StreamVariantErrcode::failed;                                          \
    }

    auto connect = this->connect.lock();
    if (!connect)
        RESP_CODE(proto::p2p_client::unknown_error)
    const auto security = connect->security_info();
    if (security.handshake != core::HandshakeState::Succeeded ||
        security.authentication != core::PeerAuthentication::ExternalPsk || req->has_token())
        RESP_CODE(proto::p2p_client::wrong_token)
    auto self_id = comm::locator::inst()->get<LocalPeerId>(connect);
    if (!self_id)
        RESP_CODE(proto::p2p_client::id_not_found)
    _self_id = self_id.value();

    const auto result = comm::locator::inst()->get<std::shared_ptr<mqas::tools::proto::p2p::NotifyConnectResult>>(connect);
    if (!result || !result.value().get() || result.value().get()->peer_id() != req->peer_id())
        RESP_CODE(proto::p2p_client::id_not_found)

    auto locator = comm::locator::inst();
    auto registry = locator->get<std::shared_ptr<DirectStreamRegistry>>(connect);
    if (!registry) {
        locator->deposit_cxt<std::shared_ptr<DirectStreamRegistry>>(connect, std::make_shared<DirectStreamRegistry>());
        registry = locator->get<std::shared_ptr<DirectStreamRegistry>>(connect);
    }
    _peer_stream_id = req->stream_id();
    _local_stream_id = _peer_stream_id;
    std::string reason;
    if (!_peer_stream_id) { resp.set_reason("missing stream identifier"); RESP_CODE(proto::p2p_client::refused_by_peer) }
    const auto& connection_registry = registry->get();
    _entry = connection_registry->reserve_peer(this->get_origin(), _peer_stream_id);
    if (!_entry) { resp.set_reason("duplicate peer stream identifier"); RESP_CODE(proto::p2p_client::already_connected) }
    const auto policy = locator->get<DirectStreamPolicy>(connect);
    if (policy && !policy->get()(this->get_origin(), _peer_stream_id, _local_stream_id, reason)) {
        if (reason.size() > 1024) reason = "stream rejected";
        resp.set_reason(reason); RESP_CODE(proto::p2p_client::refused_by_peer)
    }
    bool accepted = true;
    if (!on_connect_request_signal.empty())
        accepted = on_connect_request_signal.emit(req);

    if (!accepted)
        RESP_CODE(proto::p2p_client::refused_by_peer)

    // Callbacks may close the connection and clear its locator context.
    const auto live_registry = locator->get<std::shared_ptr<DirectStreamRegistry>>(connect);
    if (!live_registry || !live_registry->get()) RESP_CODE(proto::p2p_client::unknown_error)
    if (!live_registry->get()->complete(_entry, _local_stream_id, _peer_stream_id)) {
        resp.set_reason("invalid or duplicate stream identifier"); RESP_CODE(proto::p2p_client::already_connected)
    }

    _peer_id = req->peer_id();
    resp.set_code(proto::p2p_client::ok);
    resp.set_name(get_name());
    resp.set_peer_id(_self_id);
    resp.set_stream_id(_local_stream_id);
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
    if (!msg) return;
    _awaiting_direct_ack = false;
    LOG(INFO) << "[P2PDirectStream] on_connect_result code=" << static_cast<int>(code)
              << " ret=" << static_cast<int>(msg->code());

    const auto connect = this->connect.lock();
    const auto result = connect ? comm::locator::inst()->get<std::shared_ptr<mqas::tools::proto::p2p::NotifyConnectResult>>(connect) : std::nullopt;
    const auto registry = connect ? comm::locator::inst()->get<std::shared_ptr<DirectStreamRegistry>>(connect) : std::nullopt;
    if (code == core::StreamVariantErrcode::ok && msg->code() == proto::p2p_client::ok &&
        result && result.value().get() && result.value().get()->peer_id() == msg->peer_id() &&
        registry && registry->get() && registry->get()->complete(_entry, _local_stream_id, msg->stream_id()))
    {
        _peer_stream_id = msg->stream_id();
        _peer_id = msg->peer_id();
        _peer_name = msg->name();
        on_connected(_peer_name);
    }
    else
    {
        release_stream_id();
        on_rejected_signal.emit(msg->reason());
        on_connect_failed(msg->code() == proto::p2p_client::ok ? proto::p2p_client::refused_by_peer : msg->code());
        if (code == core::StreamVariantErrcode::ok) this->close();
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
