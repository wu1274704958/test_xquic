#include <mqas/io/udp.h>
#include "mqas/io/exception.h"

void mqas::io::UdpOp::init(std::shared_ptr<uv_udp_t> h, std::shared_ptr<uv_loop_t> l)
{
	if(const int ret = uv_udp_init(l.get(),h.get()); ret != 0)throw Exception(ret);
}

void mqas::io::UdpOp::init(std::shared_ptr<uv_udp_t> h, std::shared_ptr<uv_loop_t> l, unsigned flags)
{
	if (const int ret = uv_udp_init_ex(l.get(), h.get(),flags); ret != 0)throw Exception(ret);
}

template class mqas::io::Handle<uv_udp_t,mqas::io::UdpOp>;