#include <mqas/io/udp.h>

#include <utility>
#include "mqas/io/exception.h"

void mqas::io::UdpOp::init(std::shared_ptr<uv_udp_t> h, std::shared_ptr<uv_loop_t> l)
{
	if(const int ret = uv_udp_init(l.get(),h.get()); ret != 0)throw Exception(ret);
}

void mqas::io::UdpOp::init(std::shared_ptr<uv_udp_t> h, std::shared_ptr<uv_loop_t> l, unsigned flags)
{
	if (const int ret = uv_udp_init_ex(l.get(), h.get(),flags); ret != 0)throw Exception(ret);
}

mqas::io::UdpSocket::UdpSocket() : buffer_(1500)
{

}

void mqas::io::UdpSocket::open(OsSocket sock) const
{
	if (const int ret = uv_udp_open(handle_.get(),sock); ret != 0)throw Exception(ret);
}

void mqas::io::UdpSocket::bind(const sockaddr& addr, unsigned flags) const
{
	if (const int ret = uv_udp_bind(handle_.get(), &addr,flags); ret != 0)throw Exception(ret);
}

void mqas::io::UdpSocket::connect(const sockaddr& addr) const
{
	if (const int ret = uv_udp_connect(handle_.get(), &addr); ret != 0)throw Exception(ret);
}

void mqas::io::UdpSocket::get_peer_addr(sockaddr& name) const
{
	int addrlen = sizeof(sockaddr);
	if (const int ret = uv_udp_getpeername(handle_.get(), &name,&addrlen); ret != 0)throw Exception(ret);
}

void mqas::io::UdpSocket::get_sock_addr(sockaddr& name) const
{
	int addrlen = sizeof(sockaddr);
	if (const int ret = uv_udp_getsockname(handle_.get(), &name, &addrlen); ret != 0)throw Exception(ret);
}

void mqas::io::UdpSocket::set_broadcast(int on) const
{
	if (const int ret = uv_udp_set_broadcast(handle_.get(), on); ret != 0)throw Exception(ret);
}

void mqas::io::UdpSocket::set_ttl(int ttl) const
{
	if (const int ret = uv_udp_set_ttl(handle_.get(), ttl); ret != 0)throw Exception(ret);
}

void mqas::io::UdpSocket::send(const std::vector<std::span<uint8_t>>& d, const sockaddr& addr,
	std::function<void(UdpSocket*, int)> send_cb)
{
	send_cxts_.emplace_back(std::move(send_cb));
	UdpSendReqCxt& cxt = send_cxts_.back();
	cxt.req.data = &cxt;
	std::vector<uv_buf_t> buf(d.size());
	for (size_t i = 0; i < d.size(); ++i)
	{
		buf[i].base = reinterpret_cast<char*>(d[i].data());
		buf[i].len = static_cast<size_t>(d[i].size());
	}
	const int ret = uv_udp_send(&cxt.req, handle_.get(), buf.data(),
		static_cast<unsigned>(buf.size()), &addr, on_send_callback);
	if (ret != 0)
	{
		send_cxts_.pop_back();
		throw Exception(ret);
	}
}


int mqas::io::UdpSocket::try_send(const std::vector<std::span<uint8_t>>& d, const sockaddr& addr) const
{
	std::vector<uv_buf_t> buf(d.size());
	for (size_t i = 0; i < d.size(); ++i)
	{
		buf[i].base = reinterpret_cast<char*>(d[i].data());
		buf[i].len = static_cast<size_t>(d[i].size());
	}
	const int ret = uv_udp_try_send(handle_.get(), buf.data(), static_cast<unsigned>(buf.size()),&addr);
	if (ret < 0)
		throw Exception(ret);
	return ret;
}

void mqas::io::UdpSocket::send(const std::span<uint8_t>& d, const sockaddr& addr,
	std::function<void(UdpSocket*, int)> send_cb)
{
	send_cxts_.emplace_back(std::move(send_cb));
	UdpSendReqCxt& cxt = send_cxts_.back();
	cxt.req.data = &cxt;
	uv_buf_t buf = {};
	buf.base = reinterpret_cast<char*>(d.data());
	buf.len = static_cast<size_t>(d.size());
	const int ret = uv_udp_send(&cxt.req, handle_.get(), &buf,1,&addr,on_send_callback);
	if (ret != 0)
	{
		send_cxts_.pop_back();
		throw Exception(ret);
	}
}

int mqas::io::UdpSocket::try_send(const std::span<uint8_t>& d, const sockaddr& addr) const
{
	uv_buf_t buf = {};
	buf.base = reinterpret_cast<char*>(d.data());
	buf.len = static_cast<size_t>(d.size());
	const int ret = uv_udp_try_send(handle_.get(), &buf, 1, &addr);
	if (ret < 0)
		throw Exception(ret);
	return ret;
}

void mqas::io::UdpSocket::recv_start(
	std::function<void(UdpSocket*,const std::optional<std::span<uint8_t>>&,ssize_t nread,const sockaddr*, unsigned)> recv_cb)
{
	const int ret = uv_udp_recv_start(handle_.get(), buf_alloc_cb,[](uv_udp_t* handle,
		ssize_t nread,
		const uv_buf_t* buf,
		const struct sockaddr* addr,
		unsigned flags)
	{
		if (const auto sock = static_cast<UdpSocket*>(handle->data))
		{
			std::optional<std::span<uint8_t>> span = std::nullopt;
			if(nread > 0)
				span = std::make_optional(std::span(reinterpret_cast<uint8_t*>(buf->base),nread));
			sock->recv_cb_(sock,span,nread,addr,flags);
		}
	});
	if(ret != 0)throw Exception(ret);
	recv_cb_ = std::move(recv_cb);
}

int mqas::io::UdpSocket::using_recvmmsg() const
{
	return uv_udp_using_recvmmsg(handle_.get());
}

void mqas::io::UdpSocket::recv_stop()
{
	if (const int ret = uv_udp_recv_stop(handle_.get()); ret != 0)throw Exception(ret);
	recv_cb_ = nullptr;
}

size_t mqas::io::UdpSocket::get_send_queue_size() const
{
	return uv_udp_get_send_queue_size(handle_.get());
}

size_t mqas::io::UdpSocket::get_send_queue_count() const
{
	return uv_udp_get_send_queue_count(handle_.get());
}

void mqas::io::UdpSocket::buf_alloc_cb(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf)
{
	if(const auto sock = static_cast<UdpSocket*>(handle->data))
	{
		if(sock->buffer_.size() < suggested_size)
			sock->buffer_.resize(suggested_size);
		buf->base = reinterpret_cast<char*>(sock->buffer_.data());
		buf->len = static_cast<size_t>(sock->buffer_.size());
	}
}

void mqas::io::UdpSocket::on_send_callback(uv_udp_send_t* send, int status)
{
	const auto sock = static_cast<UdpSocket*>(send->handle->data);
	const auto cxt = static_cast<UdpSendReqCxt*>(send->data);
	if (cxt && sock)
	{
		if(cxt->cb)cxt->cb(sock, status);
		for (auto it = sock->send_cxts_.begin(); it != sock->send_cxts_.end(); )
		{
			if (&(it->req) == send)
			{
				it = sock->send_cxts_.erase(it);
				break;
			}
			++it;
		}
	}
}

mqas::io::UdpSendReqCxt::UdpSendReqCxt(std::function<void(UdpSocket*, int)> cb):req({})
{
	this->cb = std::move(cb);
}


template class mqas::io::Handle<uv_udp_t,mqas::io::UdpOp>;
