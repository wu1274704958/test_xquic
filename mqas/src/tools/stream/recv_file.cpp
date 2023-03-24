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
    int mode = req->overlay() ? O_RDWR : O_CREAT | O_RDWR;
    ret.set_code(proto::ReqSendFileRet_Code_ok);
//    if(!req->overlay() && fs::exists(req->name())) {
//        ret.set_code(proto::ReqSendFileRet_Code_already_exists);
//        core::ProtoBufMsg::write_msg<ReqSendFileMsgRetPair>(ret_buf,ret);
//        return mqas::core::StreamVariantErrcode::failed;
//    }
    open_req_.data = this;
    uv_fs_open(connect_cxt_->engine_cxt_->io_cxt->get_loop().get(),&open_req_,req->name().c_str(),mode,0666, [](uv_fs_t* req){
        auto self = static_cast<RecvFileStream*>(req->data);
        core::StreamVariantErrcode errcode = core::StreamVariantErrcode::ok;
        proto::ReqSendFileRet ret;
        ret.set_code(proto::ReqSendFileRet_Code_ok);
        if(req->result < 0)
        {
            ret.set_code(proto::ReqSendFileRet_Code_open_failed);
            ret.set_error_code((int )req->result);
            errcode = core::StreamVariantErrcode::failed;
        }else{
            self->file_ = (uv_file)req->result;
            MD5_Init(&self->md5Ctx_);
        }
        self->send_sv_msg<ReqSendFileMsgRetPair,core::stream_variant_cmd::req_use_stream_tag>(ret,self->stream_tag_,0,1,errcode);
        uv_fs_req_cleanup(req);
    });
    req_msg_ = req;
    return mqas::core::StreamVariantErrcode::skip_and_manual;
}

bool mqas::tools::RecvFileStream::is_recv_end() const {
    return recv_bytes_ >= req_msg_->size();
}

size_t mqas::tools::RecvFileStream::on_read(const std::span<const uint8_t> &current) {
    auto expect_max_size = req_msg_->size() - recv_bytes_;
    auto real_size = current.size() > expect_max_size ? expect_max_size : current.size();
    auto is_idle = write_req_.empty();
    write_req_.emplace_back(std::make_tuple<std::shared_ptr<uv_fs_t>,std::vector<uint8_t>,uv_buf_t>(std::make_shared<uv_fs_t>(),{},{}));
    auto& back = write_req_.back();
    auto& write_req = std::get<0>(back);
    auto& real_buf = std::get<1>(back);
    auto& buf = std::get<2>(back);

    real_buf.resize(real_size);
    memcpy(real_buf.data(),current.data(),real_size);
    MD5_Update(&md5Ctx_,current.data(),real_size);
    buf = uv_buf_init((char *) real_buf.data(), real_buf.size());
    recv_bytes_ += real_size;
    if(is_recv_end())
        setIsWaitPeerChangeRet(true);
    if(is_idle) {
        write_req->data = this;
        uv_fs_write(connect_cxt_->engine_cxt_->io_cxt->get_loop().get(), write_req.get(), file_, &buf, 1, -1,
                    on_write_file_cb);
    }

    return real_size;
}

mqas::core::StreamVariantErrcode
mqas::tools::RecvFileStream::on_peer_quit_msg_s(const std::shared_ptr<proto::SendFileEnd> &m,
                                                std::vector<uint8_t> &ret_buf) {
    proto::ReqSendFileRet ret;
    ret.set_code(proto::ReqSendFileRet_Code_ok);
    uint8_t md5_digest[MD5_DIGEST_LENGTH]{0};
    std::string_view md5((const char*)md5_digest,sizeof(md5_digest));
    MD5_Final(md5_digest,&md5Ctx_);
    if(md5 != m->md5())
    {
        ret.set_code(proto::ReqSendFileRet_Code_md5_not_same);
        core::ProtoBufMsg::write_msg<ReqSendFileMsgRetPair>(ret_buf,ret);
        return mqas::core::StreamVariantErrcode::failed;
    }
    is_checked_md5 = true;
    if(file_ != 0)
        return core::StreamVariantErrcode::skip_and_manual;
    return mqas::core::StreamVariantErrcode::ok;
}

void mqas::tools::RecvFileStream::on_close() {
    IStream::on_close();
    if(!write_req_.empty())
    {
        for(auto & i : write_req_)
            uv_fs_req_cleanup(std::get<0>(i).get());
        write_req_.clear();
    }
    close_file_sync();
}

void mqas::tools::RecvFileStream::close_file_sync() {
    if(file_!=0) {
        uv_fs_close(connect_cxt_->engine_cxt_->io_cxt->get_loop().get(), &close_req_, file_, nullptr);
        uv_fs_req_cleanup(&close_req_);
        file_ = 0;
    }
}

void mqas::tools::RecvFileStream::on_write_file_cb(uv_fs_t *req) {
    auto self = static_cast<RecvFileStream*>(req->data);
    if(self == nullptr) return;
    ssize_t res = req->result;
    if (res < 0) {
        proto::ReqSendFileRet ret;
        ret.set_code(proto::ReqSendFileRet_Code_write_failed);
        ret.set_error_code((int)res);
        self->send<ReqSendFileMsgRetPair>(ret);
        LOG(ERROR) << "Recv file stream write error: " << uv_strerror((int)res);
    }
    else {
        LOG(INFO) << "Recv file stream write "<< res << "bytes";
        self->write_bytes_ += res;
    }
    uv_fs_req_cleanup(req);
    if(self->write_bytes_ >= self->req_msg_->size()) {
        self->close_file_async();
    }
    for(auto it = self->write_req_.begin();it != self->write_req_.end();++it)
    {
        if(std::get<0>(*it).get() == req)
        {
            self->write_req_.erase(it);
            break;
        }
    }
    if(!self->write_req_.empty())
    {
        auto& back = self->write_req_[0];
        auto& write_req = std::get<0>(back);
        auto& buf = std::get<2>(back);
        write_req->data = self;
        uv_fs_write(self->connect_cxt_->engine_cxt_->io_cxt->get_loop().get(), write_req.get(), self->file_, &buf, 1, -1,
                    on_write_file_cb);
    }
}

void mqas::tools::RecvFileStream::close_file_async() {
    if(file_!=0) {
        close_req_.data = this;
        uv_fs_close(connect_cxt_->engine_cxt_->io_cxt->get_loop().get(), &close_req_, file_, [](uv_fs_t* req){
            auto self = static_cast<RecvFileStream*>(req->data);
            uv_fs_req_cleanup(req);
            self->file_ = 0;
            if(self->is_checked_md5)
            {
                proto::ReqSendFileRet ret;
                ret.set_code(proto::ReqSendFileRet_Code_ok);
                self->send_sv_msg<ReqSendFileMsgRetPair,core::stream_variant_cmd::req_quit_hold_stream>(ret,self->stream_tag_,0,1,core::StreamVariantErrcode::ok);
            }
        });
    }
}
