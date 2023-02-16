#include <mqas/io/ip.h>
#include <uv.h>
#include <mqas/io/exception.h>

void mqas::io::Ip::str2addr_ipv4(const char* str, int port, sockaddr& addr)
{
	if (const int ret = uv_ip4_addr(str, port, reinterpret_cast<sockaddr_in*>(&addr)); ret != 0)throw Exception(ret);
}

void mqas::io::Ip::str2addr_ipv6(const char* str, int port, sockaddr& addr)
{
	if (const int ret = uv_ip6_addr(str, port, reinterpret_cast<sockaddr_in6*>(&addr)); ret != 0)throw Exception(ret);
}

std::string mqas::io::Ip::addr2str_ipv4(const sockaddr& addr)
{
	char buf[INET_ADDRSTRLEN] = { 0 };
	uv_ip4_name(reinterpret_cast<const sockaddr_in*>(&addr), buf, sizeof(buf));
	return { buf };
}

std::string mqas::io::Ip::addr2str_ipv6(const sockaddr& addr)
{
	char buf[INET6_ADDRSTRLEN] = { 0 };
	uv_ip6_name(reinterpret_cast<const sockaddr_in6*>(&addr), buf, sizeof(buf));
	return { buf };
}

std::string mqas::io::Ip::addr2str(const sockaddr& addr)
{
	char buf[INET6_ADDRSTRLEN] = { 0 };
	uv_ip_name(&addr, buf, sizeof(buf));
	return { buf };
}