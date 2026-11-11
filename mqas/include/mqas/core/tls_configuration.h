#pragma once
#include <memory>
#include <utility>
#include <mqas/macro.h>
#include <openssl/ssl.h>

namespace mqas::core {
namespace detail { struct TlsAccess; }
enum class PeerAuthentication { None, CertificatePin, ExternalPsk };
enum class HandshakeState { Pending, Succeeded, Failed };
struct ConnectionSecurityInfo {
    HandshakeState handshake = HandshakeState::Pending;
    PeerAuthentication authentication = PeerAuthentication::None;
};

// Immutable policy metadata. An unclassified context never asserts peer authentication;
// its caller must not mutate the supplied SSL_CTX while an engine uses it.
class MQAS_EXTERN TlsConfiguration {
public:
    static std::shared_ptr<const TlsConfiguration> unclassified(std::shared_ptr<SSL_CTX> context);
    PeerAuthentication authentication() const noexcept { return authentication_; }
private:
    TlsConfiguration(std::shared_ptr<SSL_CTX> context, PeerAuthentication authentication)
        : context_(std::move(context)), authentication_(authentication) {}
    std::shared_ptr<SSL_CTX> context_;
    PeerAuthentication authentication_;
    friend struct TlsConfigurationFactory;
    friend struct detail::TlsAccess;
};
}
