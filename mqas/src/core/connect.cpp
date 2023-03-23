//
// Created by wws on 2023/2/28.
//
#include <mqas/core/connect.h>

void mqas::core::IStream::on_init(::lsquic_stream_t *lsquic_stream,connect_cxt* connect_cxt) {
    stream_ = lsquic_stream;
    connect_cxt_ = connect_cxt;
    reader_.lsqr_ctx = this;
    reader_.lsqr_read = reader_read;
    reader_.lsqr_size = reader_size;
    if(want_read_on_init_) want_read(true);
}

size_t mqas::core::IStream::do_read() {
    const auto ret = lsquic_stream_readf(stream_,read_func,this);
    if(ret == -1)
    {
        LOG(ERROR) << "Stream "<< stream_ <<" read get error " << errno;
        switch(errno)
        {
            case EBADF:
                LOG(ERROR) << "do_read() Stream "<< stream_ <<" is closed";
                want_read(false);
                break;
            case ECONNRESET:
                LOG(ERROR) << "do_read() Stream "<< stream_ <<" is reset";
                want_read(false);
                break;
        }
    }
    else if(ret == 0)
    {
        LOG(INFO) << "Stream "<< stream_ <<" EOS has been reached will be closed";
        shutdown(StreamAspect::Read);
    }
#if !NDEBUG
    else{
        LOG(INFO) << "Stream "<< stream_ <<" read " << ret << "bytes";
    }
#endif
    return ret;
}

void mqas::core::IStream::do_write() {
    const auto ret = lsquic_stream_writef(stream_,&reader_);
    if(ret == -1) {
        LOG(ERROR) << "Stream " << stream_ << " write get error " << errno;
        want_write(false);
    }
#if !NDEBUG
    else
        LOG(INFO) << "Stream "<< stream_ <<" write " << ret << "bytes";
#endif
    if(buf_write_pos == buf_.size())
    {
        buf_.clear();
        buf_write_pos = 0;
        lsquic_stream_wantwrite(stream_,0);
        flush();
    }
}

void mqas::core::IStream::on_close() {
    is_closed_ = true;
}

void mqas::core::IStream::on_reset(StreamAspect how) {
    reset_val_ = how;
}

bool mqas::core::IStream::write(const std::span<uint8_t> &data) {
    if(data.empty()) return false;
    if(!want_write(true))
        return false;
    const size_t old_len = buf_.size();
    buf_.resize(old_len + data.size());
    std::memcpy(&buf_[old_len],data.data(),data.size());
    connect_cxt_->engine_cxt_->process_conns_lazy();
    return true;
}

size_t mqas::core::IStream::reader_read(void *lsqr_ctx, void *buf, size_t count) {
    const auto self = static_cast<IStream*>(lsqr_ctx);
    const auto rsz = self->buf_.size() - self->buf_write_pos;
    auto const sz = rsz > count ? count : rsz;
    std::memcpy(buf,self->buf_.data() + self->buf_write_pos,sz);
    self->buf_write_pos += sz;
    return sz;
}

size_t mqas::core::IStream::reader_size(void *lsqr_ctx) {
    const auto self = static_cast<IStream*>(lsqr_ctx);
    return self->buf_.size() - self->buf_write_pos;
}

size_t mqas::core::IStream::read_func(void *ctx, const unsigned char *buf, size_t len, [[maybe_unused]] int fin) {
    const auto self = static_cast<IStream*>(ctx);
    if(len == 0) return len;
    const size_t old_len = self->read_buf_.size();
    self->read_buf_.resize(old_len + len);
    std::memcpy(&self->read_buf_[old_len],buf,len);
    return len;
}

bool mqas::core::IStream::want_read(bool f) const {
    if(lsquic_stream_wantread(stream_,f ? 1 : 0) == -1) {
        LOG(ERROR) << "Stream "<< stream_ <<" want read get error " << errno;
        return false;
    }
    return true;
}

bool mqas::core::IStream::close() {
    if(lsquic_stream_close(stream_) == -1) {
        LOG(ERROR) << "Stream "<< stream_ <<" close get error " << errno;
        return false;
    }
    return true;
}

bool mqas::core::IStream::want_write(bool f) const {
    if(lsquic_stream_wantwrite(stream_,f ? 1 : 0) == -1) {
        LOG(ERROR) << "Stream "<< stream_ <<" want read get error " << errno;
        return false;
    }
    return true;
}

bool mqas::core::IStream::shutdown(StreamAspect how) {
    if(lsquic_stream_shutdown(stream_,static_cast<int>(how)) == -1) {
        LOG(ERROR) << "Stream "<< stream_ <<" shutdown " << static_cast<int>(how) << " get error " << errno;
        return false;
    }
    return true;
}

void mqas::core::IStream::clear_read_buf() {
    read_buf_.clear();
    buf_read_pos = 0;
}

size_t mqas::core::IStream::on_read(const std::span<const uint8_t>& current) {return 0;}

bool mqas::core::IStream::has_unread_data() const {
    return !read_buf_.empty() && buf_read_pos < read_buf_.size();
}

std::span<const uint8_t> mqas::core::IStream::read(size_t sz) {
    size_t rsz = sz > unread_size() ? unread_size() : sz;
    if(rsz == 0) return {};
    return read_uncheck(rsz);
}

size_t mqas::core::IStream::unread_size() const {
    return read_buf_.size() - buf_read_pos;
}

std::span<const uint8_t> mqas::core::IStream::read_all() {
    size_t rsz = unread_size();
    if(rsz == 0) return {};
    return read_uncheck(rsz);
}

std::span<const uint8_t> mqas::core::IStream::read_uncheck(size_t sz) {
    std::span<const uint8_t> span(&read_buf_[buf_read_pos],sz);
    move_read_pos_uncheck(sz);
    return span;
}

void mqas::core::IStream::move_read_pos_uncheck(size_t sz) {
    buf_read_pos += sz;
    if(buf_read_pos == read_buf_.size())
    {
        buf_read_pos = 0;
        read_buf_.clear();
    }
}

void *mqas::core::IStream::get_cxt() const {
    return cxt_;
}

void mqas::core::IStream::set_cxt(void *c) {
    cxt_ = c;
}

std::span<const uint8_t> mqas::core::IStream::read_all_not_move() const {
    size_t rsz = unread_size();
    if(rsz == 0) return {};
    std::span<const uint8_t> span(&read_buf_[buf_read_pos],rsz);
    return span;
}

bool mqas::core::IStream::flush() const {
    if(lsquic_stream_flush(stream_) == -1) {
        LOG(ERROR) << "Stream "<< stream_ <<" flush get error " << errno;
        return false;
    }
    return true;
}

void mqas::core::IStream::append_unread(const std::span<uint8_t> &d) {
    const size_t old_len = read_buf_.size();
    read_buf_.resize(old_len + d.size());
    std::memcpy(&read_buf_[old_len],d.data(),d.size());
}


