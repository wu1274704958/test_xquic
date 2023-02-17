#include <mqas/context.h>
#include <mqas/io/context.h>
#include "mqas/io/timer.h"
#include <mqas/io/udp.h>
#include <mqas/io/ip.h>

using namespace mqas;

int main(int argc,const char** argv)
{
	Context<mqas::core::InitFlags::GLOBAL_SERVER> context;
	io::Context io_cxt;
	
	
	auto udp = io_cxt.make_handle<io::UdpSocket>();
	sockaddr local;
	io::Ip::str2addr_ipv4("0.0.0.0",8083,local);
	udp->bind(local, UV_UDP_REUSEADDR);
	udp->recv_start([](io::UdpSocket* sock,const std::optional<std::span<char>>& data,ssize_t nread, const sockaddr* peer, unsigned flag)
	{
		if(!data || peer == nullptr) return;
		std::string str(data->data(),data->size());
		std::string addrstr = io::Ip::addr2str(*peer);
		printf("recv %s from %s\n",str.c_str(),addrstr.c_str());
	});

	io_cxt.run_until(is_running());
	return 0;
}