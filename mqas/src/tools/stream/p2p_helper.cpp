#include "mqas/tools/stream/p2p_helper.h"
#include "mqas/tools/model/p2p_model.h"
#include "mqas/comm/locator.h"

using namespace mqas::comm;

namespace mqas::tools::p2p {

    core::StreamVariantErrcode P2PHelperStream::on_change_msg_s(const std::shared_ptr<proto::p2p::ReqConnectPeer>& msg, std::vector<uint8_t>& ret)
    {
        return core::StreamVariantErrcode::ok;
    }


    core::StreamVariantErrcode P2PHelperStream::on_change_msg_s(const std::shared_ptr<proto::p2p::ReqRespondPeerReqConnect>& msg,
        std::vector<uint8_t>& ret)
    {
        return core::StreamVariantErrcode::ok;
    }

    core::StreamVariantErrcode P2PHelperStream::on_peer_connect(uint32_t id, const proto::p2p::ClientIpList & ip)
    {
        auto model = locator::inst()->get<p2p::p2p_model>();
        if (!model)
            return core::StreamVariantErrcode::failed;
        //model.value().get().merge_id()
    }
}