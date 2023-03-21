//
// Created by wws on 2023/3/21.
//

#ifndef MQAS_TOOL_STREAM_MSG_DEF_H
#define MQAS_TOOL_STREAM_MSG_DEF_H
#include <mqas/core/pb_stream.h>
#include <mqas/tools/proto/send_file.pb.h>

namespace mqas::tools {
    using ReqSendFileMsgPair = core::PBMsgPair<1, proto::ReqSendFile>;
    using SendFileEndMsgPair = core::PBMsgPair<2, proto::SendFileEnd>;
    using ReqSendFileMsgRetPair = core::PBMsgPair<3, proto::ReqSendFileRet>;
}
#endif //MQAS_TOOL_STREAM_MSG_DEF_H
