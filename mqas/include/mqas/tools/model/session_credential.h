#pragma once

#include <mqas/macro.h>
#include <array>
#include <cstdint>

namespace mqas::tools::p2p {
// One pair of lobby peers owns one short-lived external-PSK credential.
// Generation and access are confined to the helper's I/O thread.
class MQAS_EXTERN session_credential {
public:
    session_credential() = default;
    ~session_credential();
    session_credential(const session_credential&) = delete;
    session_credential& operator=(const session_credential&) = delete;
    session_credential(session_credential&&) = delete;
    session_credential& operator=(session_credential&&) = delete;
    bool generate(uint32_t first_peer, uint32_t second_peer, uint64_t now_ms);
    void clear() noexcept;
    bool valid_at(uint64_t now_ms) const noexcept;
    const std::array<unsigned char, 32>& secret() const noexcept { return secret_; }
    const std::array<unsigned char, 16>& identity() const noexcept { return identity_; }
    uint64_t expires_at_ms() const noexcept { return expires_at_ms_; }
    uint64_t issued_at_ms() const noexcept { return issued_at_ms_; }
    uint32_t first_peer() const noexcept { return first_peer_; }
    uint32_t second_peer() const noexcept { return second_peer_; }
private:
    std::array<unsigned char, 32> secret_{};
    std::array<unsigned char, 16> identity_{};
    uint64_t issued_at_ms_ = 0, expires_at_ms_ = 0;
    uint32_t first_peer_ = 0, second_peer_ = 0;
};
}
