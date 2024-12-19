#pragma once
#include <mqas/macro.h>
#include <string>
#include <uv.h>
#include <vector>

namespace mqas::io {
	class MQAS_EXTERN Ip {
	public:
		static bool str2addr_ipv4(const char* str, int port, sockaddr& addr);
		static bool str2addr_ipv6(const char* str, int port, sockaddr& addr);
		static bool str2addr(const char* str, int port, sockaddr& addr);

		static std::string addr2str_ipv4(const sockaddr& addr);
		static std::string addr2str_ipv6(const sockaddr& addr);
		static std::string addr2str(const sockaddr& addr);
		static u_short addr_get_port(const sockaddr& addr);
		
		static bool is_valid_local_ip(const char* ip);
		static void collect_local_ip(std::vector<std::string>& res);
		static bool valid_ipv4(const char* str, int port);
		static bool valid_ipv6(const char* str, int port);
		static bool valid_ip(const char* str, int port);
	};
}