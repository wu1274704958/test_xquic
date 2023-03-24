//
// Created by Administrator on 2023/3/21.
//

#include "mqas/tools/stream/send_file.h"
#include <filesystem>
namespace fs = std::filesystem;

mqas::core::StreamVariantErrcode
mqas::tools::SendFileStream::on_change_msg_s(const std::shared_ptr<proto::ReqSendFile> &req,
                                             std::vector<uint8_t> &ret_buf) {
    if(req->name().empty() || !fs::exists(req->name()))
        return core::StreamVariantErrcode::failed;
    fs::path path = req->name();
    proto::ReqSendFile ret;
    read_req_.data = this;
    uv_fs_open(connect_cxt_->engine_cxt_->io_cxt->get_loop().get(),&read_req_,req->name().c_str(),O_RDONLY,0644, [](uv_fs_t* req){
        auto self = static_cast<SendFileStream*>(req->data);
        if(req->result < 0)
        {
            self->on_read_failed_cb_.emit(self,req->result);
            return;
        }
        self->file_ = (uv_file)req->result;
        MD5_Init(&self->md5Ctx_);
        self->send_sv_msg<ReqSendFileMsgPair,core::stream_variant_cmd::req_use_stream_tag>(*(self->req_msg_), self->stream_tag_,0,0,core::StreamVariantErrcode::ok);
        uv_fs_req_cleanup(req);
    });
    const auto filename = path.filename();
    ret.set_name(filename.string());
    ret.set_size(fs::file_size(path));
    if(ret.buf_size() < RECOMMEND_BUF_SIZE)
        ret.set_buf_size(RECOMMEND_BUF_SIZE);
    real_buf_.resize(ret.buf_size());
    // move
    req_msg_ = std::make_shared<proto::ReqSendFile>(std::move(ret));
    return mqas::core::StreamVariantErrcode::skip_and_manual;
}

void
mqas::tools::SendFileStream::on_peer_change_ret_msg_s(mqas::core::StreamVariantErrcode code,
                                                      const std::shared_ptr<proto::ReqSendFileRet>& m)
{
    if(code != core::StreamVariantErrcode::ok) {
        LOG(ERROR) << "SendFileStream peer return code = " << m->code() << " error = " << m->error_code();
        on_peer_change_ret_err_cb_.emit(this,*m);
        return;
    }else{
        buf_ = uv_buf_init((char*)real_buf_.data(),real_buf_.size());
        read_req_.data = this;
        uv_fs_read(connect_cxt_->engine_cxt_->io_cxt->get_loop().get(),&read_req_,file_,&buf_,1,-1,on_read_file_cb);
    }
}

void mqas::tools::SendFileStream::on_read_msg_s(const std::shared_ptr<proto::ReqSendFileRet>& ret)
{
    LOG(ERROR) << "SendFileStream peer quit ret code = " << ret->code() << " error = " << ret->error_code();
    on_peer_change_ret_err_cb_.emit(this,*ret);
}
void mqas::tools::SendFileStream::on_peer_quit_ret_msg_s(core::StreamVariantErrcode e,const std::shared_ptr<proto::ReqSendFileRet>& ret)
{
    if(e != core::StreamVariantErrcode::ok) {
        LOG(ERROR) << "SendFileStream peer quit ret code = " << ret->code() << " error = " << ret->error_code();
        on_peer_change_ret_err_cb_.emit(this,*ret);
        return;
    }else{
        on_send_success_cb_.emit(this);
        LOG(INFO) << "SendFileStream send success!!!";
    }
}

sigc::connection mqas::tools::SendFileStream::add_on_read_failed_cb(const typename ON_READ_FILE_FAILED_SIGNAL_T::slot_type &slot)
{
    return on_read_failed_cb_.connect(slot);
}
sigc::connection mqas::tools::SendFileStream::add_on_change_ret_err_cb(const typename ON_PEER_CHANGE_RET_ERR_SIGNAL_T::slot_type& slot)
{
    return on_peer_change_ret_err_cb_.connect(slot);
}

void mqas::tools::SendFileStream::on_close() {
    IStream::on_close();
    close_file_sync();
}

void mqas::tools::SendFileStream::on_read_file_cb(uv_fs_t *req) {
    auto self = static_cast<SendFileStream*>(req->data);
    if (req->result < 0) {
        uv_fs_req_cleanup(req);
        LOG(ERROR) << "SendFileStream read file failed " << req->result;
        self->on_read_failed_cb_.emit(self,req->result);
    }
    else if (req->result == 0) {
        uv_fs_req_cleanup(req);
        self->close_file_async();
    }
    else if (req->result > 0) {
        MD5_Update(&self->md5Ctx_,self->buf_.base,req->result);
        self->write(std::span<uint8_t>((uint8_t*)self->buf_.base,req->result));
        uv_fs_req_cleanup(req);
        self->read_req_.data = self;
        uv_fs_read(self->connect_cxt_->engine_cxt_->io_cxt->get_loop().get(),&self->read_req_,self->file_,&self->buf_,1,-1,on_read_file_cb);
    }
}

void mqas::tools::SendFileStream::close_file_sync() {
    if(file_ != 0) {
        uv_fs_close(connect_cxt_->engine_cxt_->io_cxt->get_loop().get(), &read_req_, file_, nullptr);
        uv_fs_req_cleanup(&read_req_);
        file_ = 0;
    }
}

void mqas::tools::SendFileStream::close_file_async() {
    if(file_ != 0) {
        read_req_.data = this;
        uv_fs_close(connect_cxt_->engine_cxt_->io_cxt->get_loop().get(), &read_req_, file_, [](uv_fs_t* req){
            auto self = static_cast<SendFileStream*>(req->data);
            uv_fs_req_cleanup(req);
            self->file_ = 0;
            uint8_t arr[MD5_DIGEST_LENGTH]{0};
            MD5_Final(arr,&self->md5Ctx_);
            proto::SendFileEnd end;
            end.set_name(self->req_msg_->name());
            end.set_md5(arr,sizeof (arr));
            self->send_req_quit<SendFileEndMsgPair>(self->stream_tag_,end);
        });
    }
}

sigc::connection mqas::tools::SendFileStream::add_on_success_cb(const typename ON_SEND_SUCCESS_SIGNAL_T::slot_type &slot) {
    return on_send_success_cb_.connect(slot);
}

