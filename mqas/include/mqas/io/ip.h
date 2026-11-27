#pragma once
#include <mqas/macro.h>
#include <string>
#include <cstring>
#include <uv.h>
#include <vector>

namespace mqas::io {
    // Owns the complete native endpoint. References returned by native() borrow this storage.
    struct SocketAddress {
        sockaddr_storage storage{};
        SocketAddress() = default;
        SocketAddress(const sockaddr& address) {
            if (address.sa_family == AF_INET)
                std::memcpy(&storage, &address, sizeof(sockaddr_in));
            else if (address.sa_family == AF_INET6)
                std::memcpy(&storage, &address, sizeof(sockaddr_in6));
        }
        const sockaddr& native() const { return reinterpret_cast<const sockaddr&>(storage); }
        operator const sockaddr&() const { return native(); }
    };
	class MQAS_EXTERN Ip {
	public:
		static bool str2addr_ipv4(const char* str, int port, sockaddr& addr);
		static bool str2addr_ipv6(const char* str, int port, sockaddr_storage& addr);
		static bool str2addr(const char* str, int port, sockaddr& addr);
		static bool str2addr(const char* str, int port, sockaddr_storage& addr);

		static std::string addr2str_ipv4(const sockaddr& addr);
		static std::string addr2str_ipv6(const sockaddr& addr);
		static std::string addr2str(const sockaddr& addr);
		static u_short addr_get_port(const sockaddr& addr);
		
		static bool is_valid_local_ip(const char* ip);
		static void collect_local_ip(std::vector<std::string>& res);
		static void collect_local_ip(std::vector<std::string>& res, int family);
		static void visit_local_ip(int family, void* context, void (*visitor)(void*, const char*));
		static bool is_valid_local_ip(const char* ip, int family);
		static bool valid_ipv4(const char* str, int port);
		static bool valid_ipv6(const char* str, int port);
		static bool valid_ip(const char* str, int port);

		static bool compare_ip(const sockaddr& a, const sockaddr& b,bool ignorePort = false);
	};
}