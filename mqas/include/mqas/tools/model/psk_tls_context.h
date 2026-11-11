#pragma once

#include <mqas/macro.h>
#include <cstdint>
#include <memory>
#include <span>
#include <string_view>
#include <openssl/ssl.h>
#include <mqas/core/def.h>
#include <mqas/core/tls_configuration.h>

namespace mqas::tools::p2p {
// Creates a TLS 1.3 external-PSK-only context. Both peers must supply the
// same credential, peer IDs, and ALPN. No certificate fallback is configured.
MQAS_EXTERN std::shared_ptr<const core::TlsConfiguration> make_psk_tls_context(
    bool server, std::span<const unsigned char> secret,
    std::span<const unsigned char> identity, uint32_t first_peer,
    uint32_t second_peer, std::string_view alpn);

// Initialize a peer engine only with the PSK-only context created above.
// The server's connection callback is delivered after its QUIC handshake;
// client users must also wait for LSQ_HSK_OK before exposing the connection.
template<class Engine, class Socket>
bool init_psk_peer_engine(Engine& engine, const char* config, bool server,
    std::shared_ptr<Socket> socket, std::span<const unsigned char> secret,
    std::span<const unsigned char> identity, uint32_t first_peer,
    uint32_t second_peer, std::string_view alpn) {
    auto tls = make_psk_tls_context(server, secret, identity, first_peer, second_peer, alpn);
    if (!tls) return false;
    engine.init(config, server ? core::EngineFlags::Server : core::EngineFlags::None,
        std::move(socket), std::move(tls));
    return true;
}
}
