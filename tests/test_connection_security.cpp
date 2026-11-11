#include <mqas/core/engine.h>

int main() {
    using namespace mqas::core;
    IConnect failed, succeeded;
    detail::ConnectionSecurityAccess::init(failed, PeerAuthentication::ExternalPsk);
    detail::ConnectionSecurityAccess::init(succeeded, PeerAuthentication::ExternalPsk);
    int events = 0;
    failed.on_hsk_done_signal.connect([&](IConnect&, lsquic_hsk_status) { ++events; });
    detail::ConnectionSecurityAccess::handshake_done(failed, LSQ_HSK_FAIL);
    detail::ConnectionSecurityAccess::handshake_done(succeeded, LSQ_HSK_OK);
    detail::ConnectionSecurityAccess::handshake_done(failed, LSQ_HSK_OK);
    const auto a = failed.security_info();
    const auto b = succeeded.security_info();
    return events == 1 && a.handshake == HandshakeState::Failed && a.authentication == PeerAuthentication::None &&
        b.handshake == HandshakeState::Succeeded && b.authentication == PeerAuthentication::ExternalPsk ? 0 : 1;
}
