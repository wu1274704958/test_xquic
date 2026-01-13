//
// Created by wws on 2023/2/28.
//
#include <mqas/core/connect.h>

void mqas::core::IStream::on_init(::lsquic_stream_t *lsquic_stream,connect_cxt* connect_cxt, std::weak_ptr<IConnect> connect) {
    _stream = lsquic_stream;
    connect_cxt_ = connect_cxt;
    reader_.lsqr_ctx = this;
    reader_.lsqr_read = reader_read;
    reader_.lsqr_size = reader_size;
    this->connect = std::move(connect);
    if(want_read_on_init_) want_read(true);
    lazy_timer = connect_cxt_->engine_cxt_->io_cxt.make_handle<io::Timer>();
}

size_t mqas::core::IStream::do_read() {
    const auto ret = lsquic_stream_readf(_stream,read_func,this);
    if(ret == -1)
    {
        LOG(ERROR) << "Stream "<< _stream <<" read get error " << errno;
        switch(errno)
        {
            case EBADF:
                LOG(ERROR) << "do_read() Stream "<< _stream <<" is closed";
                want_read(false);
                break;
            case ECONNRESET:
                LOG(ERROR) << "do_read() Stream "<< _stream <<" is reset";
                want_read(false);
                break;
        }
    }
    else if(ret == 0)
    {
        LOG(INFO) << "Stream "<< _stream <<" EOS has been reached will be closed";
        shutdown(StreamAspect::Read);
    }
#if !NDEBUG
    else{
        LOG(INFO) << "Stream "<< _stream <<" read " << ret << "bytes";
    }
#endif
    return ret;
}

void mqas::core::IStream::do_write() {
    const auto ret = lsquic_stream_writef(_stream,&reader_);
    if(ret == -1) {
        LOG(ERROR) << "Stream " << _stream << " write get error " << errno;
        want_write(false);
    }
#if !NDEBUG
    else
        LOG(INFO) << "Stream "<< _stream <<" write " << ret << "bytes";
#endif
    if(buf_write_pos == buf_.size())
    {
        buf_.clear();
        buf_write_pos = 0;
        want_write(false);
        flush();
    }
}

void mqas::core::IStream::on_close() {
    is_closed_ = true;
    if(lazy_timer)
        connect_cxt_->engine_cxt_->io_cxt.del_handle(lazy_timer);
    lazy_timer = nullptr;
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

void mqas::core::IStream::write_buf(const std::span<uint8_t>& data)
{
    if (data.empty()) return;
    const size_t old_len = buf_.size();
    buf_.resize(old_len + data.size());
    std::memcpy(&buf_[old_len], data.data(), data.size());
}

void mqas::core::IStream::want_write_lazy(bool v)
{
    if (buf_.empty()) return;
    if(lazy_timer_started) return;
    lazy_timer_started = true;
    lazy_timer->start([this,v](io::Timer* t){
        lazy_timer_started = false;
        t->stop();
        if (v && buf_.empty()) return;
        want_write(v);
    },0,0);
}

void mqas::core::IStream::write_lazy(const std::span<uint8_t>& data)
{
    write_buf(data);
    want_write_lazy(true);
    connect_cxt_->engine_cxt_->process_conns_lazy();
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
    if(lsquic_stream_wantread(_stream,f ? 1 : 0) == -1) {
        LOG(ERROR) << "Stream "<< _stream <<" want read get error " << errno;
        return false;
    }
    MQAS_DBG("Stream " << _stream << " want read " << f);
    return true;
}

bool mqas::core::IStream::close() {
    if(lsquic_stream_close(_stream) == -1) {
        LOG(ERROR) << "Stream "<< _stream <<" close get error " << errno;
        return false;
    }
    return true;
}

bool mqas::core::IStream::want_write(bool f) const {
    if(lsquic_stream_wantwrite(_stream,f ? 1 : 0) == -1) {
        LOG(ERROR) << "Stream "<< _stream <<" want write get error " << errno;
        return false;
    }
    MQAS_DBG("Stream " << _stream << " want write " << f);
    return true;
}

bool mqas::core::IStream::shutdown(StreamAspect how) {
    if(lsquic_stream_shutdown(_stream,static_cast<int>(how)) == -1) {
        LOG(ERROR) << "Stream "<< _stream <<" shutdown " << static_cast<int>(how) << " get error " << errno;
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

::lsquic_stream* mqas::core::IStream::get_origin() const
{
    return _stream;
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
    if(lsquic_stream_flush(_stream) == -1) {
        LOG(ERROR) << "Stream "<< _stream <<" flush get error " << errno;
        return false;
    }
    return true;
}

void mqas::core::IStream::append_unread(const std::span<uint8_t> &d) {
    const size_t old_len = read_buf_.size();
    read_buf_.resize(old_len + d.size());
    std::memcpy(&read_buf_[old_len],d.data(),d.size());
}


