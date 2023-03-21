//
// Created by wws on 2023/3/17.
//
#include <mqas/tools/stream/recv_file.h>
#include <filesystem>
namespace fs = std::filesystem;

mqas::core::StreamVariantErrcode
mqas::tools::RecvFileStream::on_change_msg_s(const std::shared_ptr<proto::ReqSendFile> &req,
                                             std::vector<uint8_t> &ret_buf) {
    proto::ReqSendFileRet ret;
    int mode = req->overlay() ? O_WRONLY : O_CREAT;
    ret.set_code(proto::ReqSendFileRet_Code_ok);
    if(!req->overlay() && fs::exists(req->name())) {
        ret.set_code(proto::ReqSendFileRet_Code_already_exists);
        core::ProtoBufMsg::write_msg<ReqSendFileMsgRetPair>(ret_buf,ret);
        return mqas::core::StreamVariantErrcode::failed;
    }
    uv_fs_t open_req;
    int fd = uv_fs_open(connect_cxt_->engine_cxt_->io_cxt->get_loop().get(),&open_req,req->name().c_str(),mode,0644, nullptr);
    if(fd < 0)
    {
        ret.set_code(proto::ReqSendFileRet_Code_open_failed);
        ret.set_error_code(fd);
        core::ProtoBufMsg::write_msg<ReqSendFileMsgRetPair>(ret_buf,ret);
        return mqas::core::StreamVariantErrcode::failed;
    }
    file_ = fd;
    req_msg_ = req;
    MD5_Init(&md5Ctx_);
    core::ProtoBufMsg::write_msg<ReqSendFileMsgRetPair>(ret_buf,ret);
    return mqas::core::StreamVariantErrcode::ok;
}

bool mqas::tools::RecvFileStream::is_recv_end() const {
    return recv_bytes_ >= req_msg_->size();
}

size_t mqas::tools::RecvFileStream::on_read(const std::span<const uint8_t> &current) {
    if(is_recv_end())
        return ProtoBufStream::on_read(current);
    real_buf_.resize(current.size());
    real_buf_.insert(real_buf_.begin(),current.begin(),current.end());
    MD5_Update(&md5Ctx_,current.data(),current.size());
    if(buf_.base != nullptr) {
        uv_fs_req_cleanup(&write_req_);
        buf_ = {};
    }
    buf_ = uv_buf_init((char *) real_buf_.data(), real_buf_.size());
    uv_fs_write(connect_cxt_->engine_cxt_->io_cxt->get_loop().get(),&write_req_,file_,&buf_,1,-1,[](uv_fs_t* req){
        auto* self = static_cast<RecvFileStream*>(req->ptr);
        if (req->result < 0) {
            proto::ReqSendFileRet ret;
            ret.set_code(proto::ReqSendFileRet_Code_write_failed);
            ret.set_error_code((int )req->result);
            self->send<ReqSendFileMsgRetPair>(ret);
            LOG(ERROR) << "Recv file stream write error: %s\n", uv_strerror((int)req->result);
        }
        else {
            LOG(INFO) << "Recv file stream write "<< req->result << "bytes";
            self->recv_bytes_ += req->result;
            if(self->is_recv_end())
                self->setIsWaitPeerChangeRet(true);
        }
    });
    write_req_.ptr = this;
    return current.size();
}

mqas::core::StreamVariantErrcode
mqas::tools::RecvFileStream::on_peer_quit_msg_s(const std::shared_ptr<proto::SendFileEnd> &m,
                                                std::vector<uint8_t> &ret_buf) {
    proto::ReqSendFileRet ret;
    ret.set_code(proto::ReqSendFileRet_Code_ok);
    if(buf_.base != nullptr) {
        uv_fs_req_cleanup(&write_req_);
        buf_ = {};
    }
    uint8_t md5_digest[MD5_DIGEST_LENGTH]{0};
    std::string_view md5((const char*)md5_digest,sizeof(md5_digest));
    MD5_Final(md5_digest,&md5Ctx_);
    int v = uv_fs_close(connect_cxt_->engine_cxt_->io_cxt->get_loop().get(),&write_req_,file_, nullptr);
    file_ = 0;
    if(v < 0)
    {
        ret.set_code(proto::ReqSendFileRet_Code_close_failed);
        ret.set_error_code(v);
        core::ProtoBufMsg::write_msg<ReqSendFileMsgRetPair>(ret_buf,ret);
        return mqas::core::StreamVariantErrcode::failed;
    }
    if(md5 != m->md5())
    {
        ret.set_code(proto::ReqSendFileRet_Code_md5_not_same);
        core::ProtoBufMsg::write_msg<ReqSendFileMsgRetPair>(ret_buf,ret);
        return mqas::core::StreamVariantErrcode::failed;
    }
    return mqas::core::StreamVariantErrcode::ok;
}

void mqas::tools::RecvFileStream::on_close() {
    IStream::on_close();
    if(buf_.base != nullptr) {
        uv_fs_req_cleanup(&write_req_);
        buf_ = {};
    }
    if(file_ != 0)
        uv_fs_close(connect_cxt_->engine_cxt_->io_cxt->get_loop().get(),&write_req_,file_, nullptr);
}
