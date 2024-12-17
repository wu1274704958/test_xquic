#include <mqas/io/ip.h>
#include <uv.h>
#include <mqas/io/exception.h>
#include <easylogging++.h>

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
	return strcmp(ip,"0.0.0.0") != 0 && strcmp(ip,"127.0.0.1") != 0;
}

void mqas::io::Ip::collect_local_ip(std::vector<std::string>& res)
{
#if _WIN32
	IP_ADAPTER_INFO adapterInfo[16];
	DWORD dwBufLen = sizeof(adapterInfo);

	DWORD dwStatus = GetAdaptersInfo(adapterInfo, &dwBufLen);
	if (dwStatus != ERROR_SUCCESS) {
		LOG(ERROR) << "GetAdaptersInfo failed" << std::endl;
		return;
	}

	IP_ADAPTER_INFO* adapter = adapterInfo;
	while (adapter) {
		IP_ADDR_STRING* ipAddr = &adapter->IpAddressList;
		while (ipAddr) {
			if(is_valid_local_ip(ipAddr->IpAddress.String))
				res.push_back(ipAddr->IpAddress.String);
			ipAddr = ipAddr->Next;
		}
		adapter = adapter->Next;
	}
#else
	struct ifaddrs* ifAddrStruct = nullptr;
	struct ifaddrs* ifa = nullptr;

	if (getifaddrs(&ifAddrStruct) == -1) {
		std::cerr << "getifaddrs failed" << std::endl;
		return;
	}

	for (ifa = ifAddrStruct; ifa != nullptr; ifa = ifa->ifa_next) {
		// Check if the address is IPv4
		if (ifa->ifa_addr->sa_family == AF_INET) {
			struct sockaddr_in* sa = reinterpret_cast<struct sockaddr_in*>(ifa->ifa_addr);
			char ipStr[INET_ADDRSTRLEN];
			inet_ntop(AF_INET, &(sa->sin_addr), ipStr, INET_ADDRSTRLEN);
			if (is_valid_ip(ipStr))
				res.push_back(ipStr);
		}
	}

	freeifaddrs(ifAddrStruct);
#endif
}

bool mqas::io::Ip::valid_ipv4(const char* str, int port)
{
	sockaddr addr;
	return uv_ip4_addr(str, port, reinterpret_cast<sockaddr_in*>(&addr)) == 0;
}
bool mqas::io::Ip::valid_ipv6(const char* str, int port)
{
	sockaddr addr;
	return uv_ip6_addr(str, port, reinterpret_cast<sockaddr_in6*>(&addr)) == 0;
}
bool mqas::io::Ip::valid_ip(const char* str, int port)
{
	if (!is_valid_local_ip(str))
		return false;
	sockaddr addr;
	auto ret = uv_ip4_addr(str, port, reinterpret_cast<sockaddr_in*>(&addr));
	if (ret != 0)
		ret = uv_ip6_addr(str, port, reinterpret_cast<sockaddr_in6*>(&addr));
	return ret == 0;
}