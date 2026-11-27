#include <mqas/io/ip.h>
#include <mqas/tools/model/p2p_model.h>
#include <vector>
#include <string>

using namespace mqas::tools::p2p;

static bool check_candidates(bool first, bool second) {
    p2p_model model;
    const auto a = model.registe_client("a", "198.51.100.1", 10001, {});
    const auto b = model.registe_client("b", "198.51.100.2", 10002, {});
    mqas::tools::proto::p2p::ClientIpList addresses[2];
    addresses[0].set_port(20001);
    addresses[1].set_port(20002);
    if (first) addresses[0].add_ip_list("10.0.0.1");
    if (second) addresses[1].add_ip_list("10.0.0.2");
    const connect_cxt* context = nullptr;
    const auto id = model.reg_context(a, b, addresses[0], {}, &context);
    model.reg_context(b, a, addresses[1], {}, &context);
    const auto [step, first_result] = model.next_cxt(id, a);
    const auto [other_step, second_result] = model.current_cxt(id, b);
    if (step != StepResult::Success || other_step != StepResult::Success || !first_result || !second_result) return false;
    const bool internal = first && second;
    return first_result->connect_data().ip() == (internal ? "10.0.0.2" : "198.51.100.2") &&
        second_result->connect_data().ip() == (internal ? "10.0.0.1" : "198.51.100.1") &&
        first_result->connect_data().port() == (internal ? 20002 : 10002) &&
        second_result->connect_data().ip_index() == (internal ? 0 : connect_cxt::ExternalIpIndex);
}

int main() {
    if (mqas::io::Ip::is_valid_local_ip(nullptr) || mqas::io::Ip::is_valid_local_ip("127.9.1.2") ||
        mqas::io::Ip::is_valid_local_ip("169.254.1.2") || mqas::io::Ip::is_valid_local_ip("invalid") ||
        !mqas::io::Ip::is_valid_local_ip("192.168.1.2")) return 1;
    sockaddr_storage ipv6{};
    sockaddr ipv4{};
    if (!mqas::io::Ip::valid_ipv6("::1", 8084) || !mqas::io::Ip::valid_ip("::1", 8084) ||
        !mqas::io::Ip::str2addr("::1", 8084, ipv6) || ipv6.ss_family != AF_INET6 ||
        mqas::io::Ip::str2addr("::1", 8084, ipv4)) return 4;
    std::vector<std::string> addresses;
    mqas::io::Ip::collect_local_ip(addresses);
    for (const auto& address : addresses) if (!mqas::io::Ip::is_valid_local_ip(address.c_str())) return 2;
    for (bool a : {false, true}) for (bool b : {false, true}) if (!check_candidates(a, b)) return 3;
    return 0;
}
