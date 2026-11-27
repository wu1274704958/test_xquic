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

enum class Outcome { IPv4, IPv6, Timeout };

static bool check_race(bool offer_both, Outcome outcome) {
    namespace pb = mqas::tools::proto::p2p;
    p2p_model model;
    const auto a = model.registe_client("a", "198.51.100.1", 10001, {});
    const auto b = model.registe_client("b", "198.51.100.2", 10002, {});
    pb::ClientIpList local, remote;
    local.set_port(20001);
    local.add_ip_list("10.1.0.1");
    remote.set_port(20002);
    remote.add_ip_list("10.1.0.2");
    for (auto ip : {"::1", "fe80::1", "2001:db8::1", "2001:db8::3"}) {
        auto* endpoint = local.add_additional_ipv6_endpoints();
        endpoint->set_ip(ip);
        endpoint->set_port(40001);
    }
    if (offer_both) {
        auto* endpoint = remote.add_additional_ipv6_endpoints();
        endpoint->set_ip("2001:db8::2");
        endpoint->set_port(40002);
    }
    const connect_cxt* state = nullptr;
    const auto id = model.reg_context(a, b, local, {}, &state);
    model.reg_context(b, a, remote, {}, &state);
    auto report = [](const pb::NotifyConnectPeerData& command, uint32_t sender, const char* source) {
        auto msg = std::make_shared<pb::ReqSubmitRecvPeerKeyCode>();
        msg->set_peer_id(sender);
        msg->set_ip_index(command.connect_data().ip_index());
        msg->set_verify_code(command.connect_data().verify_code());
        msg->mutable_peer_addr()->set_ip(source);
        msg->mutable_peer_addr()->set_port(40000);
        return msg;
    };
    auto ipv4_a = model.next_cxt(id, a).second.value();
    auto ipv4_b = model.current_cxt(id, b).second.value();
    auto from_b = report(ipv4_b, b, "198.51.100.2");
    if (model.submit_verify_code(id, a, from_b)) return false;
    if (model.start_ipv6_probe(id) != offer_both) return false;
    // Advance the lobby path to its external pair before the internal pair's late report arrives.
    auto external = model.next_cxt(id, a).second;
    if (!external || external->connect_data().ip_index() != connect_cxt::ExternalIpIndex) return false;
    if (model.submit_verify_code(id, b, report(*external, a, "198.51.100.1"))) return false;
    if (!offer_both) return model.next_cxt(id, a).first == StepResult::End;
    if (model.start_ipv6_probe(id)) return false;
    auto ipv6_a = model.generate_connect_data(*state, a, *state->ipv6_probe).value();
    auto ipv6_b = model.generate_connect_data(*state, b, *state->ipv6_probe).value();
    if (!ipv6_a.connect_data().use_non_lobby_socket() || ipv6_b.connect_data().ip() != "2001:db8::1" ||
        ipv6_a.connect_data().port() != 40002) return false;
    auto from_a = report(ipv6_a, a, "2001:db8::1");
    if (model.submit_verify_code(id, b, from_a) || model.submit_verify_code(id, b, from_a)) return false;
    auto last4 = report(ipv4_a, a, "198.51.100.1");
    auto last6 = report(ipv6_b, b, "2001:db8::2");
    if (outcome == Outcome::Timeout) {
        return model.next_cxt(id, a).first == StepResult::End && !state->winner() &&
            !model.submit_verify_code(id, a, last6) && !model.submit_verify_code(id, b, last4);
    }
    const bool ipv6 = outcome == Outcome::IPv6;
    if (!(ipv6 ? model.submit_verify_code(id, a, last6) : model.submit_verify_code(id, b, last4))) return false;
    return state->winner() == (ipv6 ? &*state->ipv6_probe : &state->lobby_probes.front()) &&
        !model.submit_verify_code(id, a, last6) && !model.submit_verify_code(id, b, last4);
}

int main() {
    if (!check_race(false, Outcome::Timeout)) return 5;
    for (auto outcome : {Outcome::IPv4, Outcome::IPv6, Outcome::Timeout})
        if (!check_race(true, outcome)) return 6;
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
