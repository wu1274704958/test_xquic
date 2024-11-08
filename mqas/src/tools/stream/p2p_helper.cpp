#include "mqas/tools/stream/p2p_helper.h"
#include "mqas/tools/model/p2p_model.h"

namespace mqas::tools::p2p {

    core::StreamVariantErrcode P2PHelperStream::on_change_msg_s(const std::shared_ptr<proto::p2p::ReqConnectPeer>& msg, std::vector<uint8_t>& ret)
    {
        return core::StreamVariantErrcode::ok;
    }

}