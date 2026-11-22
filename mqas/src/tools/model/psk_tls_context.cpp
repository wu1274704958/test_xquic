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
std::string psk_server_name(std::span<const unsigned char> identity) {
    constexpr char hex[] = "0123456789abcdef";
    std::string name = "p2p-";
    for (const auto byte : identity) {
        name += hex[byte >> 4];
        name += hex[byte & 15];
    }
    return name + ".invalid";
}
namespace {
struct AlpnPolicy {
    std::vector<unsigned char> protocol;
    bool match_credential = false;
};
void free_alpn(void*, void* data, CRYPTO_EX_DATA*, int, long, void*) {
    delete static_cast<AlpnPolicy*>(data);
}
int select_alpn(SSL* ssl, const unsigned char** out, unsigned char* out_len,
    const unsigned char* in, unsigned int in_len, void* argument) {
    const auto& policy = *static_cast<const AlpnPolicy*>(argument);
    if (policy.match_credential) {
        const auto* credential = SSL_get0_selected_credential(ssl);
        size_t length = 0;
        const auto* identity = credential ? SSL_CREDENTIAL_get0_pre_shared_key_id(credential, &length) : nullptr;
        const auto* name = SSL_get_servername(ssl, TLSEXT_NAMETYPE_host_name);
        try {
            if (!identity || !name || psk_server_name({identity, length}) != name)
                return SSL_TLSEXT_ERR_ALERT_FATAL;
        } catch (...) { return SSL_TLSEXT_ERR_ALERT_FATAL; }
    }
    const auto& protocol = policy.protocol;
    unsigned char* selected = nullptr;
    if (SSL_select_next_proto(&selected, out_len, protocol.data(),
        static_cast<unsigned int>(protocol.size()), in, in_len) != OPENSSL_NPN_NEGOTIATED)
        return SSL_TLSEXT_ERR_ALERT_FATAL;
    *out = selected;
    return SSL_TLSEXT_ERR_OK;
}
bool add_credential(SSL_CTX* context, const PskCredentialView& view) {
    auto [secret, identity, first_peer, second_peer] = view;
    if (secret.size() != 32 || identity.size() != 16 || !first_peer ||
        !second_peer || first_peer == second_peer) return false;
    if (first_peer > second_peer) std::swap(first_peer, second_peer);
    constexpr unsigned char prefix[] = "mqas-p2p-v1";
    std::array<unsigned char, sizeof(prefix) - 1 + 8> label{};
    std::copy_n(prefix, sizeof(prefix) - 1, label.begin());
    for (unsigned i = 0; i < 4; ++i) {
        label[sizeof(prefix) - 1 + i] = static_cast<unsigned char>(first_peer >> (24 - 8 * i));
        label[sizeof(prefix) + 3 + i] = static_cast<unsigned char>(second_peer >> (24 - 8 * i));
    }
    auto* credential = SSL_CREDENTIAL_new_pre_shared_key(secret.data(), secret.size(),
        identity.data(), identity.size(), EVP_sha256(), label.data(), label.size());
    if (!credential) return false;
    const bool ok = SSL_CTX_add1_credential(context, credential) == 1;
    SSL_CREDENTIAL_free(credential);
    return ok;
}

std::shared_ptr<const core::TlsConfiguration> make_context(bool server, bool match_credential,
    std::span<const PskCredentialView> credentials, std::string_view alpn) {
    if (alpn.empty() || alpn.size() > 255) return {};
    auto context = std::shared_ptr<SSL_CTX>(SSL_CTX_new(TLS_method()), SSL_CTX_free);
    if (!context || !SSL_CTX_set_min_proto_version(context.get(), TLS1_3_VERSION) ||
        !SSL_CTX_set_max_proto_version(context.get(), TLS1_3_VERSION)) return {};
    SSL_CTX_set_session_cache_mode(context.get(), SSL_SESS_CACHE_OFF);
    SSL_CTX_set_options(context.get(), SSL_OP_NO_TICKET);
    SSL_CTX_set_early_data_enabled(context.get(), 0);
    for (const auto& credential : credentials)
        if (!add_credential(context.get(), credential)) return {};
    if (server) {
        static const int index = SSL_CTX_get_ex_new_index(0, nullptr, nullptr, nullptr, free_alpn);
        auto policy = std::make_unique<AlpnPolicy>();
        policy->match_credential = match_credential;
        policy->protocol.push_back(static_cast<unsigned char>(alpn.size()));
        policy->protocol.insert(policy->protocol.end(), alpn.begin(), alpn.end());
        if (index < 0 || !SSL_CTX_set_ex_data(context.get(), index, policy.get())) return {};
        SSL_CTX_set_alpn_select_cb(context.get(), select_alpn, policy.release());
    }
    return core::TlsConfigurationFactory::psk(std::move(context));
}
}

std::shared_ptr<const core::TlsConfiguration> make_psk_tls_context(bool server,
    std::span<const unsigned char> secret, std::span<const unsigned char> identity,
    uint32_t first_peer, uint32_t second_peer, std::string_view alpn) {
    const PskCredentialView credential{secret, identity, first_peer, second_peer};
    return make_context(server, false, {&credential, 1}, alpn);
}

std::shared_ptr<const core::TlsConfiguration> make_psk_server_context(
    std::span<const PskCredentialView> credentials, std::string_view alpn) {
    return make_context(true, true, credentials, alpn);
}
}
