#include "mqas/tools/model/session_credential.h"
#include <openssl/crypto.h>
#include <openssl/rand.h>
#include <limits>

namespace mqas::tools::p2p {
session_credential::~session_credential() { clear(); }
void session_credential::clear() noexcept {
    OPENSSL_cleanse(secret_.data(), secret_.size());
    OPENSSL_cleanse(identity_.data(), identity_.size());
    first_peer_ = second_peer_ = 0;
    issued_at_ms_ = expires_at_ms_ = 0;
}
bool session_credential::generate(uint32_t first_peer, uint32_t second_peer, uint64_t now_ms) {
    clear();
    constexpr uint64_t lifetime_ms = 120000;
    if (!first_peer || !second_peer || first_peer == second_peer ||
        now_ms > std::numeric_limits<uint64_t>::max() - lifetime_ms) return false;
    if (RAND_bytes(secret_.data(), secret_.size()) != 1 ||
        RAND_bytes(identity_.data(), identity_.size()) != 1) { clear(); return false; }
    first_peer_ = first_peer;
    second_peer_ = second_peer;
    issued_at_ms_ = now_ms;
    expires_at_ms_ = now_ms + lifetime_ms;
    return true;
}
bool session_credential::valid_at(uint64_t now_ms) const noexcept {
    return first_peer_ && second_peer_ && expires_at_ms_ && now_ms >= issued_at_ms_ && now_ms < expires_at_ms_;
}
}
