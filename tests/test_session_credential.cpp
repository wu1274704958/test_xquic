#include <mqas/tools/model/session_credential.h>
#include <algorithm>
#include <cstdio>
#include <limits>

int main() {
    using mqas::tools::p2p::session_credential;
    session_credential a, b;
    if (a.valid_at(1000) || a.generate(0, 2, 1000) || a.generate(1, 1, 1000) ||
        a.generate(1, 2, std::numeric_limits<uint64_t>::max())) return 1;
    if (!a.generate(10, 20, 1000) || !b.generate(10, 20, 1000)) return 2;
    if (a.first_peer() != 10 || a.second_peer() != 20 || a.issued_at_ms() != 1000 ||
        a.expires_at_ms() != 121000 || a.valid_at(999) || !a.valid_at(1000) ||
        !a.valid_at(120999) || a.valid_at(121000)) return 3;
    if (a.secret() == b.secret() || a.identity() == b.identity()) return 4;
    a.clear();
    if (a.valid_at(1000) || std::any_of(a.secret().begin(), a.secret().end(), [](unsigned char c) { return c != 0; }) ||
        std::any_of(a.identity().begin(), a.identity().end(), [](unsigned char c) { return c != 0; })) return 5;
    std::puts("session credential: PASS");
    return 0;
}
