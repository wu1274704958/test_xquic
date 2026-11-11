#include <mqas/tools/model/psk_tls_context.h>
#include <algorithm>
#include <array>
#include <vector>
#include <openssl/evp.h>

namespace mqas::core {
struct TlsConfigurationFactory {
    static std::shared_ptr<const TlsConfiguration> psk(std::shared_ptr<SSL_CTX> context) {
        return std::shared_ptr<const TlsConfiguration>(new TlsConfiguration(std::move(context), PeerAuthentication::ExternalPsk));
    }
};
}
namespace mqas::tools::p2p {
namespace {
int select_alpn(SSL*, const unsigned char** out, unsigned char* out_len,
    const unsigned char* in, unsigned int in_len, void* argument) {
    const auto& protocol = *static_cast<const std::vector<unsigned char>*>(argument);
    unsigned char* selected = nullptr;
    if (SSL_select_next_proto(&selected, out_len, protocol.data(),
        static_cast<unsigned int>(protocol.size()), in, in_len) != OPENSSL_NPN_NEGOTIATED)
        return SSL_TLSEXT_ERR_ALERT_FATAL;
    *out = selected;
    return SSL_TLSEXT_ERR_OK;
}
}

std::shared_ptr<const core::TlsConfiguration> make_psk_tls_context(bool server,
    std::span<const unsigned char> secret, std::span<const unsigned char> identity,
    uint32_t first_peer, uint32_t second_peer, std::string_view alpn) {
    if (secret.size() != 32 || identity.size() != 16 || first_peer == 0 ||
        second_peer == 0 || first_peer == second_peer || alpn.empty() || alpn.size() > 255)
        return {};
    if (first_peer > second_peer) std::swap(first_peer, second_peer);
    constexpr unsigned char prefix[] = "mqas-p2p-v1";
    std::array<unsigned char, sizeof(prefix) - 1 + 8> label{};
    std::copy_n(prefix, sizeof(prefix) - 1, label.begin());
    for (unsigned i = 0; i < 4; ++i) {
        label[sizeof(prefix) - 1 + i] = static_cast<unsigned char>(first_peer >> (24 - 8 * i));
        label[sizeof(prefix) + 3 + i] = static_cast<unsigned char>(second_peer >> (24 - 8 * i));
    }
    auto protocol = std::make_shared<std::vector<unsigned char>>();
    protocol->reserve(alpn.size() + 1);
    protocol->push_back(static_cast<unsigned char>(alpn.size()));
    protocol->insert(protocol->end(), alpn.begin(), alpn.end());
    auto context = std::shared_ptr<SSL_CTX>(SSL_CTX_new(TLS_method()),
        [protocol](SSL_CTX* ctx) { SSL_CTX_free(ctx); });
    if (!context || !SSL_CTX_set_min_proto_version(context.get(), TLS1_3_VERSION) ||
        !SSL_CTX_set_max_proto_version(context.get(), TLS1_3_VERSION)) return {};
    SSL_CTX_set_session_cache_mode(context.get(), SSL_SESS_CACHE_OFF);
    SSL_CTX_set_options(context.get(), SSL_OP_NO_TICKET);
    SSL_CTX_set_early_data_enabled(context.get(), 0);
    auto* credential = SSL_CREDENTIAL_new_pre_shared_key(secret.data(), secret.size(),
        identity.data(), identity.size(), EVP_sha256(), label.data(), label.size());
    if (!credential) return {};
    const bool ok = SSL_CTX_add1_credential(context.get(), credential) == 1;
    SSL_CREDENTIAL_free(credential);
    if (!ok) return {};
    if (server) SSL_CTX_set_alpn_select_cb(context.get(), select_alpn, protocol.get());
    return core::TlsConfigurationFactory::psk(std::move(context));
}
}
