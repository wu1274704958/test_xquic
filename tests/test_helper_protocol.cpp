#include <mqas/tools/stream/p2p_helper.h>
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
    return 0;
}
