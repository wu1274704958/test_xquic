//
// Created by Administrator on 2023/3/21.
//

#ifndef MQAS_TOOL_STREAM_SEND_FILE_HPP
#define MQAS_TOOL_STREAM_SEND_FILE_HPP

#include "MsgDef.h"
#include <uv.h>
#include <atomic>
#include <openssl/md5.h>
#include <sigc++/sigc++.h>

namespace mqas::tools{
    class MQAS_EXTERN SendFileStream : public core::ProtoBufStream<SendFileStream,ReqSendFileMsgPair,SendFileEndMsgPair,ReqSendFileMsgRetPair>{
    public:
        using ON_PEER_CHANGE_RET_ERR_SIGNAL_T = sigc::signal<void(SendFileStream*,const proto::ReqSendFileRet&)>;
        using ON_READ_FILE_FAILED_SIGNAL_T = sigc::signal<void(SendFileStream*,ssize_t)>;
        using ON_SEND_SUCCESS_SIGNAL_T = sigc::signal<void(SendFileStream*)>;
        static constexpr size_t RECOMMEND_BUF_SIZE = 2048;
        core::StreamVariantErrcode on_change_msg_s(const std::shared_ptr<proto::ReqSendFile>& req,
                                                   std::vector<uint8_t> &ret_buf);
        void on_peer_change_ret_msg_s(mqas::core::StreamVariantErrcode code,const std::shared_ptr<proto::ReqSendFileRet>& m);
        sigc::connection add_on_read_failed_cb(const typename ON_READ_FILE_FAILED_SIGNAL_T::slot_type &);
        sigc::connection add_on_change_ret_err_cb(const typename ON_PEER_CHANGE_RET_ERR_SIGNAL_T::slot_type &);
        sigc::connection add_on_success_cb(const typename ON_SEND_SUCCESS_SIGNAL_T::slot_type &);
        void on_read_msg_s(const std::shared_ptr<proto::ReqSendFileRet>& ret);
        void on_peer_quit_ret_msg_s(core::StreamVariantErrcode e,const std::shared_ptr<proto::ReqSendFileRet>& ret);
        void close_file_sync();
        void close_file_async();
        void on_close();
        static void on_read_file_cb(uv_fs_t*);
    protected:
        ON_PEER_CHANGE_RET_ERR_SIGNAL_T on_peer_change_ret_err_cb_;
        ON_READ_FILE_FAILED_SIGNAL_T on_read_failed_cb_;
        ON_SEND_SUCCESS_SIGNAL_T on_send_success_cb_;
        std::vector<uint8_t > real_buf_;
        std::shared_ptr<proto::ReqSendFile> req_msg_;
        uv_file file_ = 0;
        uv_fs_t read_req_ = {};
        uv_buf_t buf_ = {};
        MD5_CTX md5Ctx_ = {};
    };
}


#endif //MQAS_TOOL_STREAM_SEND_FILE_HPP
