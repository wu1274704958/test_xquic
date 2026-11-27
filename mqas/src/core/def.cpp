#include "mqas/core/def.h"
#include "mqas/io/ip.h"
#include <stdexcept>

//engine config deserialize
mqas::core::engine_config toml::from<mqas::core::engine_config>::from_toml(const value& v)
{
    mqas::core::engine_config f;
    f.bind_ip = find_or<std::string			>(v, "bind_ip", "0.0.0.0");
    const auto port = find_or<int64_t>(v, "port", 8083);
    if (port < 0 || port > 65535) throw std::out_of_range("engine_config.port must be between 0 and 65535");
    f.port = static_cast<uint16_t>(port);
    f.log_level = find_or<std::string		>(v, "log_level", "warning");
    f.log_path = find_or<std::string		>(v, "log_path", "log.txt");
    f.log_config = find_or<std::string		>(v, "log_config", "");
    f.alpn = find_or<std::string			>(v, "alpn", "");
    f.ssl_cert_path = find_or<std::string		>(v, "ssl_cert_path", "");
    f.ssl_key_path = find_or<std::string		>(v, "ssl_key_path", "");
    return f;
}


size_t std::hash<sockaddr>::operator()(const sockaddr& addr) const {

    size_t h = std::hash<int>{}(addr.sa_family);
    auto combine = [&h](size_t value) { h ^= value + 0x9e3779b9 + (h << 6) + (h >> 2); };
    if (addr.sa_family == AF_INET) {
        const auto& v4 = reinterpret_cast<const sockaddr_in&>(addr);
        combine(v4.sin_addr.s_addr);
        combine(v4.sin_port);
    } else if (addr.sa_family == AF_INET6) {
        const auto& v6 = reinterpret_cast<const sockaddr_in6&>(addr);
        for (auto byte : v6.sin6_addr.s6_addr) combine(byte);
        combine(v6.sin6_port);
        combine(v6.sin6_scope_id);
    }

    return h;
}

bool operator==(const sockaddr& a, const sockaddr& b)
{
    return mqas::io::Ip::compare_ip(a,b);
}