#pragma once
#include "mqas/io/ip.h"
#include "mqas/io/udp.h"
#ifdef MQAS_TEST_IPV6_LOOPBACK
#include "../../../../tests/ipv6_loopback_fixture.hpp"
#endif

namespace mqas::tools::p2p {
inline bool valid_probe_ipv6(const sockaddr_in6& addr) {
#ifdef MQAS_TEST_IPV6_LOOPBACK
	if (loopback_probe_fixture() && IN6_IS_ADDR_LOOPBACK(&addr.sin6_addr)) return true;
#endif
	return (addr.sin6_addr.s6_addr[0] & 0xe0) == 0x20;
}

inline bool preferred_ipv6_endpoint(io::Context& io, io::SocketAddress& local) {
#ifdef MQAS_TEST_IPV6_LOOPBACK
	if (loopback_probe_fixture()) return io::Ip::str2addr(fixture_source_address(), 0, local.storage);
#endif
	// UDP connect asks the OS for its preferred global source without transmitting.
	auto route = io.make_shared<io::UdpSocket>();
	io::SocketAddress destination;
	io::Ip::str2addr("2001:4860:4860::8888", 53, destination.storage);
	route->connect(destination);
	route->get_sock_addr(local.storage);
	auto& source = reinterpret_cast<sockaddr_in6&>(local.storage);
	source.sin6_port = 0;
	return source.sin6_family == AF_INET6 && valid_probe_ipv6(source);
}
}
