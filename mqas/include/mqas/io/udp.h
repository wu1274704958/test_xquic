#pragma once
#include <mqas/macro.h>
#include <uv.h>
#include <memory>
#include <functional>
#include <mqas/io/handle.h>
#include <span>
#include <vector>
#include <string>

namespace mqas::io
{
	class MQAS_EXTERN UdpOp
	{
	public:
		static void init(std::shared_ptr<uv_udp_t>, std::shared_ptr<uv_loop_t>);
		static void init(std::shared_ptr<uv_udp_t>, std::shared_ptr<uv_loop_t>, unsigned int flags);
	};

	struct UdpSendReqCxt;
	class MQAS_EXTERN UdpSocket : public Handle<uv_udp_t, UdpOp>
	{
		using OsSocket = uv_os_sock_t;
	public:
		UdpSocket();
		UdpSocket(UdpSocket&&) = default;
		UdpSocket(const UdpSocket&) = delete;
		UdpSocket& operator=(UdpSocket&&) = default;
		UdpSocket& operator=(const UdpSocket&) = delete;
		void open(OsSocket sock) const;
		void bind(const sockaddr& addr,unsigned int flags) const;
		void connect(const sockaddr& addr) const;
		void get_peer_addr(sockaddr& name) const;
		void get_sock_addr(sockaddr& name) const;
		void set_broadcast(int on) const;
		void set_ttl(int ttl) const;
		void send(const std::vector<std::span<char>>& data,const sockaddr& addr,
			std::function<void(UdpSocket*,int)> send_cb);
		int try_send(const std::vector<std::span<char>>& data, const sockaddr& addr) const;
		void send(const std::span<char>& data, const sockaddr& addr, std::function<void(UdpSocket*, int)> send_cb);
		int try_send(const std::span<char>& data, const sockaddr& addr) const;
		void recv_start(std::function<void(UdpSocket*,const std::span<char>&, const sockaddr*, unsigned)> recv_cb);
		int using_recvmmsg() const;
		void recv_stop();
		size_t get_send_queue_size() const;
		size_t get_send_queue_count() const;
	protected:
		static void buf_alloc_cb(uv_handle_t* handle,size_t suggested_size,uv_buf_t* buf);
		static void on_send_callback(uv_udp_send_t* send, int status);
	protected:
		std::list<UdpSendReqCxt> send_cxts_;
		std::function<void(UdpSocket*,const std::span<char>&,const sockaddr*,unsigned)> recv_cb_;
		std::vector<char> buffer_;
	};
	struct MQAS_EXTERN UdpSendReqCxt
	{
		uv_udp_send_t req;
		std::function<void(UdpSocket*, int)> cb;
		UdpSendReqCxt(std::function<void(UdpSocket*, int)> cb);
	};
}
