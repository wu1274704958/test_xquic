#pragma once

#include "mqas/tools/stream/p2p_helper_client.h"
#include "mqas/core/proto/simple.h"
#include "../model/p2p_ipv6_endpoint.hpp"

namespace mqas::tools::p2p {
class P2PProbePath {
public:
	P2PProbePath(io::Context& io, uint32_t self) : io_(io), id(self) {}
	virtual ~P2PProbePath() { finish(); }
	virtual void prepare(proto::p2p::ClientIpList&) = 0;

	sigc::signal<void(const proto::p2p::ReqSubmitRecvPeerKeyCode&)> on_probe_received;

	void start(const std::shared_ptr<proto::p2p::NotifyConnectPeerData>& msg) {
		stop();
		if (!socket_ || !msg->connect_data().send_times() ||
			!io::Ip::str2addr(msg->connect_data().ip().c_str(), msg->connect_data().port(), destination_.storage)) return;
		if (!timer_) timer_ = io_.make_shared<io::Timer>();
		auto bytes = verify_packet(msg->connect_data());
		if (!bytes) return;
		packet_ = std::move(*bytes);
		remaining_ = msg->connect_data().send_times();
		timer_->start(std::bind_front(&P2PProbePath::tick, this), 0, msg->connect_data().send_delay());
	}
	void stop() {
		if (timer_ && timer_->is_start()) timer_->stop();
		packet_.clear();
	}
	std::shared_ptr<io::UdpSocket> finish() {
		stop();
		receive_.disconnect();
		return std::move(socket_);
	}
protected:
	void listen(std::shared_ptr<io::UdpSocket> socket) {
		socket_ = std::move(socket);
		receive_ = socket_->on_recv_signal.connect(sigc::mem_fun(*this, &P2PProbePath::receive));
	}
	io::Context& io_;
	const io::UdpSocket& socket() const { return *socket_; }
private:
	void tick(io::Timer*) {
		try { socket_->try_send(packet_, destination_); }
		catch (const io::Exception&) { /* Retry with the next scheduled probe. */ }
		if (--remaining_ == 0) stop();
	}
	void receive(io::UdpSocket*, const std::optional<std::span<uint8_t>>& buf, ssize_t nread,
		const sockaddr* addr, unsigned) {
		if (!buf || nread != core::proto::simple_pkg<uint32_t>::SIZE_WITHOUT_PARAMS + 10 || !addr) return;
		auto [pkg, size] = core::proto::simple_pkg<uint32_t>::parse_command(*buf);
		if (!pkg) return;
		auto msg = try_parse_verify_msg(*pkg, addr);
		if (msg && msg->peer_id() != id) on_probe_received.emit(*msg);
	}
	std::shared_ptr<io::UdpSocket> socket_;
	sigc::connection receive_;
	std::shared_ptr<io::Timer> timer_;
	std::vector<uint8_t> packet_;
	io::SocketAddress destination_;
	uint32_t id, remaining_ = 0;
	std::optional<std::vector<uint8_t>> verify_packet(const proto::p2p::ConnectPeerData& probe) const {
		std::array<uint8_t, 10> body{};
		std::span<uint8_t> bytes(body);
		comm::to_big_endian(probe.verify_code(), bytes);
		comm::to_big_endian(id, bytes, 4);
		comm::to_big_endian(static_cast<uint16_t>(probe.ip_index()), bytes, 8);
		core::proto::simple_pkg<uint32_t> pkg;
		pkg.body = body;
		return pkg.generate();
	}

	std::optional<proto::p2p::ReqSubmitRecvPeerKeyCode> try_parse_verify_msg(
		const core::proto::simple_pkg<uint32_t>& pkg, const sockaddr* addr) const {
		if (pkg.body.size() != 10) return {};
		proto::p2p::ReqSubmitRecvPeerKeyCode msg;
		msg.set_verify_code(comm::from_big_endian<uint32_t>(pkg.body));
		msg.set_peer_id(comm::from_big_endian<uint32_t>(pkg.body, 4));
		msg.set_ip_index(comm::from_big_endian<uint16_t>(pkg.body, 8));
		auto* observed = msg.mutable_peer_addr();
		observed->set_ip(io::Ip::addr2str(*addr));
		observed->set_port(io::Ip::addr_get_port(*addr));
		return msg;
	}

};

class LobbySocketProbePath final : public P2PProbePath {
public:
	LobbySocketProbePath(io::Context& io, uint32_t self, std::shared_ptr<io::UdpSocket> socket)
		: P2PProbePath(io, self) { listen(std::move(socket)); }
	static void describe(const sockaddr& local, proto::p2p::ClientIpList& res) {
        auto add = [](void* context, const char* ip) {
            auto& addresses = *static_cast<proto::p2p::ClientIpList*>(context);
            if (std::find(addresses.ip_list().begin(), addresses.ip_list().end(), ip) == addresses.ip_list().end())
                addresses.add_ip_list(ip);
        };
        const auto local_ip = mqas::io::Ip::addr2str(local);
        if (local_ip == "0.0.0.0" || local_ip == "::") {
            mqas::io::Ip::visit_local_ip(local.sa_family, &res, add);
        } else if (mqas::io::Ip::is_valid_local_ip(local_ip.c_str(), local.sa_family)) {
            add(&res, local_ip.c_str());
        }
		res.set_port(mqas::io::Ip::addr_get_port(local));
	}
	void prepare(proto::p2p::ClientIpList& list) override {
		if (list.port()) return; // Preserve addresses supplied by the lobby implementation.
		io::SocketAddress local;
		socket().get_sock_addr(local.storage);
		describe(local.native(), list);
	}
};

class IPv6SocketProbePath final : public P2PProbePath {
public:
	using P2PProbePath::P2PProbePath;
	void prepare(proto::p2p::ClientIpList& list) override {
		list.clear_additional_ipv6_endpoints();
		try {
			io::SocketAddress local;
			if (!preferred_ipv6_endpoint(io_, local)) return;
			auto socket = io_.make_shared<io::UdpSocket>();
			socket->bind(local, UV_UDP_IPV6ONLY);
			socket->get_sock_addr(local.storage);
			socket->recv_start();
			listen(std::move(socket));
			auto* endpoint = list.add_additional_ipv6_endpoints();
			endpoint->set_ip(io::Ip::addr2str(local.native()));
			endpoint->set_port(io::Ip::addr_get_port(local.native()));
		} catch (const io::Exception&) {
			// No usable IPv6 route on this interface.
		}
	}
};
}
