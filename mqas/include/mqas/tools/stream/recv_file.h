//
// Created by wws on 2023/3/17.
//

#ifndef MQAS_TOOL_STREAM_RECV_FILE_H
#define MQAS_TOOL_STREAM_RECV_FILE_H

#include <mqas/core/pb_stream.h>
#include <mqas/tools/proto/send_file.pb.h>

namespace mqas::tools{
    using ReqSendFileMsgPair = core::PBMsgPair<1,proto::ReqSendFile>;
    using SendFileMd5MsgPair = core::PBMsgPair<2,proto::SendFileMd5>;
    class RecvFileStream : public core::ProtoBufStream<ReqSendFileMsgPair,SendFileMd5MsgPair>{
    public:
    };
}

#endif //MQAS_TOOL_STREAM_RECV_FILE_H
