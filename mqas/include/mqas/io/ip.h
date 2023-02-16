#pragma once
#include <mqas/macro.h>
#include <string>
#include <uv.h>

namespace mqas::io {
	class MQAS_EXTERN Ip {
	public:
		static void str2addr_ipv4(const char* str, int port, sockaddr& addr);
		static void str2addr_ipv6(const char* str, int port, sockaddr& addr);
		static std::string addr2str_ipv4(const sockaddr& addr);
		static std::string addr2str_ipv6(const sockaddr& addr);
		static std::string addr2str(const sockaddr& addr);
	};
}