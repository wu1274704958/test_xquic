#include <mqas/tools/stream/p2p_helper.h>
#include <mqas/tools/model/p2p_model.h>
#include <memory>
#include <vector>

int main() {
    mqas::tools::p2p::P2PHelperStream helper;
    std::vector<uint8_t> response;
    auto request = std::make_shared<mqas::tools::proto::p2p::ReqConnectPeer>();
    auto reply = std::make_shared<mqas::tools::proto::p2p::ReqRespondPeerReqConnect>();
    if (helper.on_change_msg_s(request, response) != mqas::core::StreamVariantErrcode::failed) return 1;
    if (helper.on_change_msg_s(reply, response) != mqas::core::StreamVariantErrcode::failed) return 2;
    request->set_auth_version(2);
    reply->set_auth_version(2);
    if (helper.on_change_msg_s(request, response) != mqas::core::StreamVariantErrcode::failed) return 3;
    if (helper.on_change_msg_s(reply, response) != mqas::core::StreamVariantErrcode::failed) return 4;
    using namespace mqas::tools;
    p2p::connect_cxt context;
    context.pid = {10, 20}; context.request_initiator = 20;
    if (context.select_server() != 10) return 5;
    context.server_preferences = {proto::p2p::SERVER_VOLUNTEER, proto::p2p::SERVER_NO_PREFERENCE};
    if (context.select_server() != 10) return 6;
    context.server_preferences = {proto::p2p::SERVER_VOLUNTEER, proto::p2p::SERVER_REUSABLE};
    if (context.select_server() != 20) return 7;
    context.server_preferences = {proto::p2p::SERVER_REUSABLE, proto::p2p::SERVER_REUSABLE};
    if (context.select_server() != 20) return 8;
    context.request_initiator = 10;
    if (context.select_server() != 10) return 9;
    p2p::p2p_model model;
    const auto first = model.registe_client("first", "127.0.0.1", 1234, {});
    const auto second = model.registe_client("second", "127.0.0.1", 1235, {});
    const std::string negotiation(16, 'a');
    model.track_ready(negotiation, first, second, UINT64_MAX);
    if (model.consume_ready(negotiation, second, first, 1)) return 10;
    if (model.consume_ready(std::string(16, 'b'), first, second, 1)) return 11;
    if (!model.consume_ready(negotiation, first, second, 1)) return 12;
    if (model.consume_ready(negotiation, first, second, 1)) return 13;
    model.track_ready(negotiation, first, second, UINT64_MAX);
    if (!model.unregiste_client(second)) return 14;
    if (model.consume_ready(negotiation, first, second, 1)) return 15;
    return 0;
}
