#include <mqas/core/tls_configuration.h>

namespace mqas::core {
std::shared_ptr<const TlsConfiguration> TlsConfiguration::unclassified(std::shared_ptr<SSL_CTX> context) {
    if (!context) return {};
    return std::shared_ptr<const TlsConfiguration>(new TlsConfiguration(std::move(context), PeerAuthentication::None));
}
}
