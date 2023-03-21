//
// Created by wws on 2023/3/17.
//

#ifndef MQAS_TOOL_STREAM_RECV_FILE_H
#define MQAS_TOOL_STREAM_RECV_FILE_H

#include "MsgDef.h"
#include <uv.h>
#include <atomic>
#include <openssl/md5.h>

namespace mqas::tools{
    class MQAS_EXTERN RecvFileStream : public core::ProtoBufStream<ReqSendFileMsgPair,SendFileEndMsgPair,ReqSendFileMsgRetPair>{
    public:
        core::StreamVariantErrcode on_change_msg_s(const std::shared_ptr<proto::ReqSendFile>& req,
                                                   std::vector<uint8_t> &ret_buf);
        size_t on_read(const std::span<const uint8_t>& current);
        [[nodiscard]] bool is_recv_end() const;
        mqas::core::StreamVariantErrcode on_peer_quit_msg_s(const std::shared_ptr<proto::SendFileEnd>& m,
                                                            std::vector<uint8_t>& buf);
        void on_close();
    protected:
        std::shared_ptr<proto::ReqSendFile> req_msg_;
        std::vector<uint8_t> real_buf_;
        uv_file file_ = 0;
        uv_fs_t write_req_ = {nullptr};
        uv_buf_t buf_ = {};
        uint64_t recv_bytes_ = 0;
        MD5_CTX md5Ctx_ = {};
    };
}

#endif //MQAS_TOOL_STREAM_RECV_FILE_H
