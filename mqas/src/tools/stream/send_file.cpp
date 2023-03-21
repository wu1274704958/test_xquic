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
    uv_fs_t  open_req = {};
    int fd = uv_fs_open(connect_cxt_->engine_cxt_->io_cxt->get_loop().get(),&open_req,req->name().c_str(),O_RDONLY,0644, nullptr);
    if(fd < 0)
        return mqas::core::StreamVariantErrcode::failed;
    file_ = fd;
    ret.set_size(fs::file_size(path));
    const auto filename = path.filename();
    ret.set_name(filename.string());
    if(ret.buf_size() < RECOMMEND_BUF_SIZE)
        ret.set_buf_size(RECOMMEND_BUF_SIZE);
    core::ProtoBufMsg::write_msg<ReqSendFileMsgPair>(ret_buf,ret);
    real_buf_.resize(ret.buf_size());
    MD5_Init(&md5Ctx_);
    // move
    req_msg_ = std::make_shared<proto::ReqSendFile>(std::move(ret));
    return mqas::core::StreamVariantErrcode::ok;
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
        uv_fs_read(connect_cxt_->engine_cxt_->io_cxt->get_loop().get(),&read_req_,file_,&buf_,1,-1,[](uv_fs_t* req){
            auto self = static_cast<SendFileStream*>(req->ptr);
            if (req->result < 0) {
                LOG(ERROR) << "SendFileStream read file failed " << req->result;
                self->on_read_failed_cb_.emit(req->result);
                self->close();
            }
            else if (req->result == 0) {
                uint8_t arr[MD5_DIGEST_LENGTH]{0};
                MD5_Final(arr,&self->md5Ctx_);
                proto::SendFileEnd end;
                end.set_name(self->req_msg_->name());
                end.set_md5(arr,sizeof (arr));
                self->send_req_quit<SendFileEndMsgPair>(self->stream_tag_,end);
            }
            else if (req->result > 0) {
                MD5_Update(&self->md5Ctx_,self->buf_.base,self->buf_.len);
                self->write(std::span<uint8_t>((uint8_t*)self->buf_.base,self->buf_.len));
            }
        });
        read_req_.ptr = this;
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
        LOG(ERROR) << "SendFileStream send success!!!";
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
    if(buf_.base != nullptr) {
        uv_fs_req_cleanup(&read_req_);
        buf_ = {};
    }
    if(file_ != 0)
        uv_fs_close(connect_cxt_->engine_cxt_->io_cxt->get_loop().get(),&read_req_,file_, nullptr);
}

