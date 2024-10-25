//
// Created by wws on 2023/3/21.
//

#ifndef MQAS_TOOL_STREAM_MSG_DEF_H
#define MQAS_TOOL_STREAM_MSG_DEF_H
#include <mqas/core/pb_stream.h>
#include <mqas/tools/proto/send_file.pb.h>
#include <mqas/tools/proto/p2p.pb.h>

namespace mqas::tools {
    using ReqSendFileMsgPair = core::PBMsgPair<1, proto::ReqSendFile>;
    using SendFileEndMsgPair = core::PBMsgPair<2, proto::SendFileEnd>;
    using ReqSendFileMsgRetPair = core::PBMsgPair<3, proto::ReqSendFileRet>;
}
namespace mqas::tools::p2p {
    //p2p lobby
    using ReqRegistePeerPair = core::PBMsgPair<4, proto::p2p::ReqRegistePeer>;
    using RespondRegistePeerPair = core::PBMsgPair<5, proto::p2p::RespondRegistePeer>;
    using ReqUnregistePeerPair = core::PBMsgPair<6, proto::p2p::ReqUnregistePeer>;
    using RespondUnregistePeerPair = core::PBMsgPair<7, proto::p2p::RespondUnregistePeer>;
    using ReqPeerListPair = core::PBMsgPair<8, proto::p2p::ReqPeerList>;
    using RespondPeerListPair = core::PBMsgPair<9, proto::p2p::RespondPeerList>;
    using ReqConnectPeerPair = core::PBMsgPair<10, proto::p2p::ReqConnectPeer>;
    using RespondConnectPeerPair = core::PBMsgPair<11, proto::p2p::RespondConnectPeer>;
}
#endif //MQAS_TOOL_STREAM_MSG_DEF_H
