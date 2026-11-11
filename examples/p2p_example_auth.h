#pragma once
#include <mqas/tools/model/psk_tls_context.h>
#include <mqas/core/engine.h>
#include <mqas/tools/proto/p2p.pb.h>
#include <openssl/crypto.h>
#include <chrono>
#include <memory>
#include <span>
#include <toml.hpp>

namespace p2p_example {
inline void clear_secret(mqas::tools::proto::p2p::NotifyConnectResult& result) {
    auto* secret = result.mutable_psk_secret();
    if (!secret->empty()) OPENSSL_cleanse(secret->data(), secret->size());
    result.clear_psk_secret();
}
inline bool valid_time(const mqas::tools::proto::p2p::NotifyConnectResult& result) {
    const auto now = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    return now >= 0 && result.psk_issued_at_ms() &&
        static_cast<uint64_t>(now) >= result.psk_issued_at_ms() &&
        static_cast<uint64_t>(now) < result.psk_expires_at_ms();
}

inline std::shared_ptr<const mqas::core::TlsConfiguration> make_tls(
    mqas::tools::proto::p2p::NotifyConnectResult& result, uint32_t self, const char* config_path) {
    // Clear the protobuf copy on every exit, including parse exceptions.
    struct ClearSecret {
        mqas::tools::proto::p2p::NotifyConnectResult& result;
        ~ClearSecret() { clear_secret(result); }
    } clear{result};
    if (result.ret() != mqas::tools::proto::p2p::RetCode::ok || !self || !result.peer_id() ||
        self == result.peer_id() || !valid_time(result) || result.psk_secret().size() != 32 ||
        result.psk_identity().size() != 16) return {};
    const auto config = toml::parse(config_path);
    const auto alpn = toml::find<std::string>(config, "engine_config", "alpn");
    const auto& secret = result.psk_secret();
    const auto& identity = result.psk_identity();
    return mqas::tools::p2p::make_psk_tls_context(result.is_server(),
        std::span(reinterpret_cast<const unsigned char*>(secret.data()), secret.size()),
        std::span(reinterpret_cast<const unsigned char*>(identity.data()), identity.size()),
        self, result.peer_id(), alpn);
}

inline bool authenticated(const mqas::core::IConnect& conn,
    const mqas::tools::proto::p2p::NotifyConnectResult& result) {
    const auto security = conn.security_info();
    return security.handshake == mqas::core::HandshakeState::Succeeded &&
        security.authentication == mqas::core::PeerAuthentication::ExternalPsk && valid_time(result);
}
}
