#include <mqas/context.h>
#include <mqas/io/context.h>
#include "mqas/io/timer.h"
#include <mqas/io/udp.h>
using namespace mqas;

int main(int argc,const char** argv)
{
	Context<mqas::core::InitFlags::GLOBAL_CLIENT> context;
	io::Context io_cxt;

	auto udp = io_cxt.make_handle<io::UdpSocket>();
	sockaddr local,peer;
	io::UdpSocket::str2addr_ipv4("0.0.0.0",8084,local);
	io::UdpSocket::str2addr_ipv4("127.0.0.1", 8083, peer);
	udp->bind(local, UV_UDP_REUSEADDR);

	std::string str = "hello!!!";
	std::span<char> d(str.data(),str.size());
	int r = udp->try_send(d, peer);
	printf("try send %d\n",r);

	std::string strb1 = "hello!!!  bufs 1\n";
	std::string strb2 = "hello!!!  bufs 2\n";
	std::string strb3 = "hello!!!  bufs 3\n";
	std::vector<std::span<char>> bufs={
		{strb1.data(),strb1.size()},
		{strb2.data(),strb2.size()},
		{strb3.data(),strb3.size()}
	};

	r = udp->try_send(bufs,peer);
	printf("try send %d\n", r);
	

	std::string str2 = "hello!!! async";
	std::span<char> d2(str2.data(), str2.size());
	udp->send(d2, peer, [](io::UdpSocket* sock, int st)
	{
		printf("send state = %d\n", st);
	});

	udp->send(bufs, peer, [](io::UdpSocket* sock, int st)
	{
		printf("send state = %d\n", st);
	});


	io_cxt.run_until(is_running());
	return 0;
}