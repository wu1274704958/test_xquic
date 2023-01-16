#pragma once
#include <mqas/macro.h>
#include <uv.h>
#include <memory>
#include <functional>
#include <mqas/io/handle.h>

namespace mqas::io
{
	class MQAS_EXTERN UdpOp
	{
	public:
		static void init(std::shared_ptr<uv_udp_t>, std::shared_ptr<uv_loop_t>);
		static void init(std::shared_ptr<uv_udp_t>, std::shared_ptr<uv_loop_t>, unsigned int flags);
	};
	class MQAS_EXTERN UdpSocket : public Handle<uv_udp_t, UdpOp>
	{
	public:
		UdpSocket() = default;
		UdpSocket(UdpSocket&&) = default;
		UdpSocket(const UdpSocket&) = delete;
		UdpSocket& operator=(UdpSocket&&) = default;
		UdpSocket& operator=(const UdpSocket&) = delete;

	protected:
	};
}
