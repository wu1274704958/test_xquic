#include <mqas/core/engine.h>
namespace mqas::core{
	void IConnect::init(::lsquic_conn_t* conn, engine_cxt* engine_cxt)
	{
		this->conn_ = conn;
		this->engine_cxt_ = engine_cxt;
	}
	void IConnect::on_close() {}
	void IConnect::on_new_stream(::lsquic_stream_t* lsquic_stream) {}
	void IConnect::on_stream_read(::lsquic_stream_t* lsquic_stream) {}
	void IConnect::on_stream_write(::lsquic_stream_t* lsquic_stream) {}
	void IConnect::on_stream_close(::lsquic_stream_t* lsquic_stream){}

//optional callback
	void IConnect::on_goaway_received()
	{
		goaway_receive_ = true;
	}
	size_t IConnect::datagram_buf_size() const
	{
		return datagram_buf_.size() - datagram_buf_write_p_;
	}
	ssize_t IConnect::on_dg_write(void* buf, size_t sz)
	{
		if(datagram_queue_.empty())
			return 0;
		size_t dg_sz = datagram_queue_.front();
		if (dg_sz > sz)
		{
			datagram_queue_.pop();
			LOG(WARNING) << "skip this datagram! cause by size "<< dg_sz << " too large, safe size is" << sz; 
			datagram_buf_write_p_ = datagram_buf_write_p_ + dg_sz;
			return on_dg_write(buf,sz);
		}
		std::memcpy(buf,datagram_buf_.data() + datagram_buf_write_p_,dg_sz);
		LOG(INFO) << "datagram write " << dg_sz << "bytes";
		datagram_buf_write_p_ = datagram_buf_write_p_ + dg_sz;
		datagram_queue_.pop();
		if (datagram_buf_write_p_ == datagram_buf_.size() || datagram_queue_.empty())
		{
			datagram_buf_write_p_ = 0;
			datagram_buf_.clear();
			::lsquic_conn_want_datagram_write(conn_,0);
		}
		return dg_sz;
	}
	void IConnect::on_datagram(const void* buf, size_t){}
	void IConnect::on_hsk_done(enum lsquic_hsk_status s)
	{
		hsk_status_ = s;
	}
	void IConnect::on_new_token(const unsigned char* token, size_t token_size){}
void IConnect::on_stream_reset(lsquic_stream_t* s, int how){}
void IConnect::on_conncloseframe_received(int app_error, uint64_t error_code, const char* reason, int reason_len){}
//interface
void IConnect::set_cxt(void* d)
{
	cxt_ = d;
}
void* IConnect::get_cxt() const
{
	return cxt_;
}
bool IConnect::write_datagram(const std::span<char>& data)
{
	if(data.empty()) return false;
	if (::lsquic_conn_want_datagram_write(conn_, 1) == -1)
		return false;
	const auto old_len = datagram_buf_.size();
	datagram_buf_.resize(old_len + data.size());
	std::memcpy(&datagram_buf_[old_len],data.data(),data.size());
	datagram_queue_.push(static_cast<short>(data.size()));
	engine_cxt_->process_conns();
	return true;
}
bool IConnect::flush_datagram() const
{
	if (!datagram_queue_.empty()) {
		if (::lsquic_conn_want_datagram_write(conn_, 1) != -1)
			return true;
	}
	return false;
}
bool IConnect::write_stream(::lsquic_stream_t*, const std::span<char>& data){ return false;}

::lsquic_hsk_status IConnect::get_hsk_status() const
{
	return hsk_status_;
}
}