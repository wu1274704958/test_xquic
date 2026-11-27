#include <mqas/io/ip.h>
#include <uv.h>
#include <mqas/io/exception.h>
#include <easylogging++.h>
#include <algorithm>

bool mqas::io::Ip::str2addr_ipv4(const char* str, int port, sockaddr& addr)
{
	return uv_ip4_addr(str, port, reinterpret_cast<sockaddr_in*>(&addr)) == 0;
}

bool mqas::io::Ip::str2addr_ipv6(const char* str, int port, sockaddr_storage& addr)
{
	return uv_ip6_addr(str, port, reinterpret_cast<sockaddr_in6*>(&addr)) == 0;
}

bool mqas::io::Ip::str2addr(const char* str, int port, sockaddr& addr)
{
	return str2addr_ipv4(str, port, addr);
}

bool mqas::io::Ip::str2addr(const char* str, int port, sockaddr_storage& addr)
{
	if (uv_ip4_addr(str, port, reinterpret_cast<sockaddr_in*>(&addr)) == 0) return true;
	return str2addr_ipv6(str, port, addr);
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

u_short mqas::io::Ip::addr_get_port(const sockaddr& addr)
{
	if (addr.sa_family == AF_INET) {
		auto* addr_in = (const struct sockaddr_in*)&addr;
		return ntohs(addr_in->sin_port);
	}
	else {
		auto* addr_in6 = (const struct sockaddr_in6*)&addr;
		return ntohs(addr_in6->sin6_port);
	}
}

bool mqas::io::Ip::compare_ip(const sockaddr& a, const sockaddr& b, bool ignorePort)
{
	if (a.sa_family != b.sa_family)
		return false;
	if (a.sa_family == AF_INET) {
		auto* addra_in = (const struct sockaddr_in*)&a;
		auto* addrb_in = (const struct sockaddr_in*)&b;
		auto addr_same = std::memcmp(&addra_in->sin_addr, &addrb_in->sin_addr,sizeof(addrb_in->sin_addr)) == 0;
		return ignorePort ? addr_same : addr_same && addra_in->sin_port == addrb_in->sin_port;
	}
	else if(a.sa_family == AF_INET6) {
		auto* addra_in = (const struct sockaddr_in6*)&a;
		auto* addrb_in = (const struct sockaddr_in6*)&b;
		auto addr_same = addra_in->sin6_scope_id == addrb_in->sin6_scope_id &&
            std::memcmp(&addra_in->sin6_addr, &addrb_in->sin6_addr, sizeof(addrb_in->sin6_addr)) == 0;
		return ignorePort ? addr_same : addr_same && addra_in->sin6_port == addrb_in->sin6_port;
	}
	return false;
}

#if _WIN32
#include <winsock2.h>
#include <iphlpapi.h>
#else
#include <ifaddrs.h>
#include <arpa/inet.h>
#include <cstring>  // For memset
#include <netinet/in.h>  // For sockaddr_in
#endif


bool mqas::io::Ip::is_valid_local_ip(const char* ip) {
	if (!ip) return false;
	sockaddr_in address{};
	if (uv_ip4_addr(ip, 0, &address) != 0) return false;
	const auto host = ntohl(address.sin_addr.s_addr);
	return host != 0 && (host >> 24) != 127 && (host >> 16) != 0xa9fe &&
		(host >> 28) != 0xe && host != 0xffffffff;
}

bool mqas::io::Ip::is_valid_local_ip(const char* ip, int family)
{
    if (family == AF_INET) return is_valid_local_ip(ip);
    if (!ip || family != AF_INET6 || std::strchr(ip, '%')) return false;
    sockaddr_in6 address{};
    if (uv_ip6_addr(ip, 0, &address) != 0) return false;
    const auto* bytes = address.sin6_addr.s6_addr;
    // Global unicast and unique local addresses do not require an interface scope.
    return (bytes[0] & 0xe0) == 0x20 || (bytes[0] & 0xfe) == 0xfc;
}

void mqas::io::Ip::collect_local_ip(std::vector<std::string>& res)
{
    collect_local_ip(res, AF_INET);
}

void mqas::io::Ip::collect_local_ip(std::vector<std::string>& res, int family)
{
	uv_interface_address_t* interfaces = nullptr;
	int count = 0;
	const int error = uv_interface_addresses(&interfaces, &count);
	if (error != 0) {
		LOG(ERROR) << "uv_interface_addresses failed: " << uv_strerror(error);
		return;
	}
	for (int i = 0; i < count; ++i) {
		const auto& entry = interfaces[i];
		if (entry.is_internal || entry.address.address4.sin_family != family) continue;
		char ip[INET6_ADDRSTRLEN]{};
		const int result = family == AF_INET ? uv_ip4_name(&entry.address.address4, ip, sizeof(ip))
			: uv_ip6_name(&entry.address.address6, ip, sizeof(ip));
		if (result == 0 && is_valid_local_ip(ip, family) && std::find(res.begin(), res.end(), ip) == res.end())
			res.emplace_back(ip);
	}
	uv_free_interface_addresses(interfaces, count);
}

bool mqas::io::Ip::valid_ipv4(const char* str, int port)
{
	sockaddr addr;
	return uv_ip4_addr(str, port, reinterpret_cast<sockaddr_in*>(&addr)) == 0;
}
bool mqas::io::Ip::valid_ipv6(const char* str, int port)
{
	sockaddr_in6 addr{};
	return uv_ip6_addr(str, port, &addr) == 0;
}

bool mqas::io::Ip::valid_ip(const char* str, int port)
{
	return valid_ipv4(str, port) || valid_ipv6(str, port);
}
