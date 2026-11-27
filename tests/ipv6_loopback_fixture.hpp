#pragma once
#include <cstdlib>
#include <string_view>
namespace mqas::tools::p2p {
inline const char* fixture_source_address() {
    const auto* value = std::getenv("MQAS_TEST_IPV6_LOOPBACK");
    return value && std::string_view(value) == "bind-failure" ? "2001:db8::1" : "::1";
}
inline bool loopback_probe_fixture() { return std::getenv("MQAS_TEST_IPV6_LOOPBACK") != nullptr; }
}
