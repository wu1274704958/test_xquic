#include "mqas/tools/model/p2p_model.h"
#include "mqas/io/ip.h"
#include "p2p_ipv6_endpoint.hpp"
#include <random>
#include <chrono>
#include "mqas/core/stream.h"
#include <boost/uuid/uuid_generators.hpp>

//lobby
uint32_t mqas::tools::p2p::p2p_model::registe_client(const std::string& name, const std::string& ip, uint16_t port, std::weak_ptr<mqas::core::IStreamVariant> stream)
{
	auto id = next_id();
	if (id == 0)
		return 0;
	client_map.insert({ id, peer_data(id, name, ip, port,stream) });
	if (id < min_id)
		update_min_id();
	else if (id > max_id)
		update_max_id();
	return id;
}

bool mqas::tools::p2p::p2p_model::unregiste_client(uint32_t id)
{
	std::erase_if(ready_sessions, [id](const auto& entry) { return entry.second.server == id || entry.second.client == id; });
	auto it = this->client_map.find(id);
	if (it != this->client_map.end())
	{
		this->client_map.erase(id);
		if (id == min_id)
			update_min_id();
		else if (id == max_id)
			update_max_id();
		return true;
	}
	return false;
}

void mqas::tools::p2p::p2p_model::visit_client(std::function<void(const peer_data&)> f) const
{
	if (!f) return;
	for (const auto it : this->client_map)
	{
		f(it.second);
	}
}

const mqas::tools::p2p::peer_data* mqas::tools::p2p::p2p_model::operator[](uint32_t id) const
{
	if(client_map.contains(id))
		return &client_map.at(id);
	return nullptr;
}

uint32_t mqas::tools::p2p::p2p_model::next_id()
{
	if (min_id > 0 && min_id - 1 > 0)
		return min_id - 1;
	if (max_id + 1 == std::numeric_limits<uint32_t>::max())
		return 0;
	return max_id + 1;
}

// helper
namespace mqas::tools::p2p {
	void p2p_model::track_ready(const std::string& negotiation, uint32_t server, uint32_t client, uint64_t expires_ms)
	{
		const auto now = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
		std::erase_if(ready_sessions, [now](const auto& entry) { return now < 0 || entry.second.expires_ms <= static_cast<uint64_t>(now); });
		ready_sessions.insert_or_assign(negotiation, ready_entry{server, client, expires_ms});
	}
	bool p2p_model::consume_ready(const std::string& negotiation, uint32_t server, uint32_t client, uint64_t now_ms)
	{
		const auto it = ready_sessions.find(negotiation);
		if (it == ready_sessions.end() || it->second.server != server || it->second.client != client ||
			it->second.expires_ms <= now_ms || !(*this)[server] || !(*this)[client]) return false;
		ready_sessions.erase(it);
		return true;
	}
	bool p2p_model::set_server_preference(uint64_t id, uint32_t self, proto::p2p::ServerPreference preference, bool initiator)
	{
		auto* cxt = get_context(id);
		if (!cxt || (self != cxt->pid[0] && self != cxt->pid[1]) ||
			preference < proto::p2p::SERVER_NO_PREFERENCE || preference > proto::p2p::SERVER_REUSABLE) return false;
		cxt->server_preferences[self_idx(cxt->pid, self)] = preference;
		if (initiator) cxt->request_initiator = self;
		return true;
	}

	uint64_t p2p_model::reg_context(uint32_t self, uint32_t oth, const proto::p2p::ClientIpList& self_ip, 
		std::weak_ptr<mqas::core::IStreamVariant> self_stream, const connect_cxt** out)
	{
		auto client_self = (*this)[self];
		auto client_oth = (*this)[oth];
		*out = nullptr;
		if (client_self == nullptr || client_oth == nullptr)
			return 0;
		auto id = merge_id(self, oth);
		auto [it, inserted] = cxt_map.try_emplace(id);
		auto& cxt = it->second;
		if (inserted) {
			cxt.id = id;
		} else if (cxt.state != ConnectState::Idle || cxt.port_list[self_idx(cxt.pid, self)] != 0) {
			return 0;
		}
		set_cxt(cxt, first_peer(*client_self, *client_oth), second_peer(*client_self, *client_oth),
			self, self_ip,std::move(self_stream));
		*out = &cxt;
		return id;
	}
	int p2p_model::unreg_context(uint32_t self, uint32_t oth)
	{
		auto id = merge_id(self, oth);
		auto cxt = get_context(id);
		if (cxt == nullptr)
			return -1;
		auto idx = self_idx(cxt->pid, self);
		cxt->stream[idx].reset();
		if (!exist_context_peer(id, oth))
		{
			clear_context(id);
			return 0;
		}
		return 1;
	}
	bool p2p_model::exist_context_peer(uint64_t mid, uint32_t id) const
	{
		auto cxt = get_context_const(mid);
		if (cxt == nullptr)
			return false;
		auto idx = self_idx(cxt->pid, id);
		auto ptr = cxt->stream[idx].lock();
		return (bool)ptr;
	}
	const connect_cxt* p2p_model::get_context_const(uint64_t id) const
	{
		if (cxt_map.contains(id))
			return &(cxt_map.at(id));
		return nullptr;
	}
	const connect_cxt* p2p_model::get_context_const(uint32_t a, uint32_t b) const
	{
		return get_context_const(merge_id(a, b));
	}
	uint64_t p2p_model::merge_id(uint32_t a, uint32_t b)
	{
		auto min = std::min(a,b);
		auto max = std::max(a,b);
		uint64_t res = max;
		res |= (static_cast<uint64_t>(min) << (sizeof(uint32_t) * 8));
		return res;
	}

	static void collect_candidates(std::vector<std::string>& target, const proto::p2p::ClientIpList& list,
		const std::string& observed_address)
	{
		target.clear();
		const auto family = observed_address.find(':') == std::string::npos ? AF_INET : AF_INET6;
		for (const auto& ip : list.ip_list()) {
			if (target.size() == 32) break;
			if (ip.find('\0') == std::string::npos && mqas::io::Ip::is_valid_local_ip(ip.c_str(), family) &&
				std::find(target.begin(), target.end(), ip) == target.end()) target.push_back(ip);
		}
	}

	bool p2p_model::init_cxt(connect_cxt& cxt, const peer_data& a, const peer_data& b, const proto::p2p::ClientIpList& a_ip,
		const proto::p2p::ClientIpList& b_ip) const
	{
		cxt.is_same_external = a.ip == b.ip;
		cxt.port_list[0] = a_ip.port();
		cxt.port_list[1] = b_ip.port();
		cxt.pid[0] = a.id;
		cxt.pid[1] = b.id;
		collect_candidates(cxt.ip_list[0], a_ip, a.ip);
		collect_candidates(cxt.ip_list[1], b_ip, b.ip);
		cxt.state = ConnectState::Idle;
		cxt.stage_1 = cxt.stage_2[0] = cxt.stage_2[1] =  -1;
		return true;
	}

	bool p2p_model::set_cxt(connect_cxt& cxt, const peer_data& a, const peer_data& b, uint32_t id, const proto::p2p::ClientIpList& self_ip,
		std::weak_ptr<mqas::core::IStreamVariant> self_stream) const
	{
		cxt.is_same_external = a.ip == b.ip;
		cxt.pid[0] = a.id;
		cxt.pid[1] = b.id;
		const auto idx = a.id == id ? 0 : 1;
		//const auto oth_idx = idx == 0 ? 1 : 0;
		cxt.port_list[idx] = self_ip.port();
		cxt.additional_ipv6[idx].reset();
		for (const auto& endpoint : self_ip.additional_ipv6_endpoints()) {
			sockaddr_in6 addr{};
			if (endpoint.port() && endpoint.port() <= UINT16_MAX &&
				endpoint.ip().find('\0') == std::string::npos &&
				uv_ip6_addr(endpoint.ip().c_str(), 0, &addr) == 0 &&
				valid_probe_ipv6(addr)) {
				cxt.additional_ipv6[idx] = endpoint;
				break;
			}
		}
		collect_candidates(cxt.ip_list[idx], self_ip, idx == 0 ? a.ip : b.ip);
		cxt.stage_1 = cxt.stage_2[idx] = -1;
		cxt.state =  (cxt.port_list[0] > 0 && cxt.port_list[1] > 0) ? ConnectState::Ready : ConnectState::Idle;
		cxt.stream[idx] = std::move(self_stream);
		return true;
	}

	void p2p_model::clear_context(uint64_t mid)
	{
		cxt_map.erase(mid);
	}

	const peer_data& p2p_model::first_peer(const peer_data& a, const peer_data& b) const
	{
		auto min = std::min(a.id, b.id);
		return min == a.id ? a : b;
	}
	const peer_data& p2p_model::second_peer(const peer_data& a, const peer_data& b) const
	{
		auto max = std::max(a.id, b.id);
		return max == a.id ? a : b;
	}

	connect_cxt* p2p_model::get_context(uint64_t id)
	{
		auto it = cxt_map.find(id);
		if (it != cxt_map.end())
			return &(it->second);
		return nullptr;
	}
	connect_cxt* p2p_model::get_context(uint32_t a, uint32_t b)
	{
		return get_context(merge_id(a, b));
	}

	std::pair<StepResult, std::optional<proto::p2p::NotifyConnectPeerData>> p2p_model::next_cxt(uint64_t id,uint32_t self)
	{
		auto cxt = get_context(id);
		if (cxt == nullptr)
			return std::make_pair<StepResult, std::optional<proto::p2p::NotifyConnectPeerData>>(StepResult::None, {});
		auto res = next_cxt(*cxt);
		if (res == StepResult::Success)
			return { res,generate_connect_data(*cxt,self,cxt->lobby_probes.back())};
		return { res,{} };
	}

	std::pair<StepResult, std::optional<proto::p2p::NotifyConnectPeerData>> p2p_model::current_cxt(uint64_t id, uint32_t self) const
	{
		auto cxt = get_context_const(id);
		if (cxt == nullptr)
			return std::make_pair<StepResult, std::optional<proto::p2p::NotifyConnectPeerData>>(StepResult::None, {});
		switch (cxt->state)
		{
		case ConnectState::Idle:
		case ConnectState::ChangeToExternal:
		case ConnectState::Ready:
			return { StepResult::None,{} };
		case ConnectState::TryInternal:
		case ConnectState::TryExternal:
			return { StepResult::Success,generate_connect_data(*cxt,self,cxt->lobby_probes.back()) };
		case ConnectState::Success:
		case ConnectState::Wait:
			return {StepResult::End,{}};
		}
		return { StepResult::None,{} };
	}
	StepResult p2p_model::next_cxt(connect_cxt& cxt) const
	{
		switch (cxt.state)
		{
		case ConnectState::Idle:
			return StepResult::None;
		case ConnectState::Ready:
		case ConnectState::ChangeToExternal: {
			cxt.stage_1 = 0;
			cxt.stage_2[0] = cxt.stage_2[1] = 0;
			cxt.state = cxt.state == ConnectState::Ready && !cxt.ip_list[0].empty() && !cxt.ip_list[1].empty()
				? ConnectState::TryInternal : ConnectState::TryExternal;
			break;
		}
		case ConnectState::TryInternal:
		{
			auto& a = cxt.stage_2[0];
			auto& b = cxt.stage_2[1];
			const bool more_a = static_cast<size_t>(a + 1) < cxt.ip_list[0].size();
			const bool more_b = static_cast<size_t>(b + 1) < cxt.ip_list[1].size();
			if (!more_a && !more_b) {
				cxt.state = ConnectState::ChangeToExternal;
				return next_cxt(cxt);
			}
			if (more_a) ++a;
			if (more_b) ++b;
			break;
		}
		case ConnectState::TryExternal:
			cxt.state = ConnectState::Wait;
			return StepResult::End;
		case ConnectState::Success:
		case ConnectState::Wait:
			return StepResult::End;
		}
		auto& probe = cxt.lobby_probes.emplace_back();
		probe.path = cxt.state == ConnectState::TryExternal ? ProbePath::LobbyExternal : ProbePath::LobbyInternal;
		for (size_t i = 0; i < 2; ++i)
			probe.index[i] = cxt.state == ConnectState::TryExternal ? connect_cxt::ExternalIpIndex : cxt.stage_2[i];
		generate_verify_code(probe.code);
		return StepResult::Success;
	}

	bool p2p_model::start_ipv6_probe(uint64_t id)
	{
		auto* cxt = get_context(id);
		if (!cxt || cxt->lobby_probes.empty() || cxt->state == ConnectState::Success || cxt->state == ConnectState::Wait ||
			cxt->ipv6_probe || !cxt->additional_ipv6[0] || !cxt->additional_ipv6[1]) return false;
		auto& probe = cxt->ipv6_probe.emplace();
		probe.path = ProbePath::AdditionalIPv6;
		probe.index.fill(connect_cxt::AdditionalIPv6Index);
		generate_verify_code(probe.code);
		return true;
	}

	std::optional<proto::p2p::NotifyConnectPeerData> p2p_model::generate_connect_data(const connect_cxt& cxt, uint32_t self, const ProbeAttempt& probe) const
	{
		auto oth = other(cxt.pid, self);
		auto idx_oth = oth_idx(cxt.pid,self);
		auto self_data = (*this)[self];
		auto oth_data = (*this)[oth];
		if (!self_data || !oth_data) return {};
		proto::p2p::NotifyConnectPeerData data;
		data.set_peer_id(oth);
		auto* d = data.mutable_connect_data();
		const auto ip_idx = probe.index[idx_oth];
		if (probe.path == ProbePath::AdditionalIPv6) {
			const auto& endpoint = *cxt.additional_ipv6[idx_oth];
			d->set_ip(endpoint.ip());
			d->set_port(endpoint.port());
			d->set_use_non_lobby_socket(true);
		} else {
			if ((self_data->ip.find(':') == std::string::npos) != (oth_data->ip.find(':') == std::string::npos)) return {};
			if (probe.path == ProbePath::LobbyInternal) {
				if (ip_idx < 0 || static_cast<size_t>(ip_idx) >= cxt.ip_list[idx_oth].size()) return {};
				d->set_ip(cxt.ip_list[idx_oth][ip_idx]);
				d->set_port(cxt.port_list[idx_oth]);
			} else {
				d->set_ip(oth_data->ip);
				d->set_port(oth_data->port);
			}
		}
		d->set_verify_code(probe.code[idx_oth]);
		d->set_send_times(3);
		d->set_send_delay(200);
		d->set_ip_index(ip_idx);
		
		return {data};
	}

	void p2p_model::generate_verify_code(std::array<uint32_t, 2>& arr) const
	{
		std::random_device rd; 
		std::mt19937 gen(rd());
		std::uniform_int_distribution<uint32_t> dist(std::numeric_limits<uint32_t>::min() + 1, std::numeric_limits<uint32_t>::max());

		arr[0] = dist(gen);
		arr[1] = dist(gen);
	}

	bool p2p_model::submit_verify_code(uint64_t merge_id, uint32_t id, const std::shared_ptr<proto::p2p::ReqSubmitRecvPeerKeyCode>& msg)
	{
		auto cxt = get_context(merge_id);
		if (!cxt || !msg || (id != cxt->pid[0] && id != cxt->pid[1]) ||
			(cxt->state != ConnectState::TryInternal && cxt->state != ConnectState::TryExternal)) return false;
		const auto receiver = self_idx(cxt->pid, id);
		auto matches = [&](const ProbeAttempt& attempt) {
			return attempt.index[receiver] == msg->ip_index() && attempt.code[receiver] == msg->verify_code();
		};
		if (msg->peer_id() != other(cxt->pid, id)) return false;
		ProbeAttempt* probe = nullptr;
		if (msg->ip_index() == connect_cxt::AdditionalIPv6Index) {
			if (cxt->ipv6_probe && matches(*cxt->ipv6_probe)) probe = &*cxt->ipv6_probe;
		} else {
			auto it = std::find_if(cxt->lobby_probes.begin(), cxt->lobby_probes.end(), matches);
			if (it != cxt->lobby_probes.end()) probe = &*it;
		}
		if (!probe || probe->replies[receiver]) return false;
		const auto& observed = msg->peer_addr();
		if (!observed.port() || observed.port() > UINT16_MAX || observed.ip().find('\0') != std::string::npos) return false;
		if (probe->path == ProbePath::AdditionalIPv6) {
			sockaddr_in6 addr{};
			if (uv_ip6_addr(observed.ip().c_str(), 0, &addr) || !valid_probe_ipv6(addr)) return false;
		} else {
			const auto* sender = (*this)[msg->peer_id()];
			if (!sender) return false;
			sockaddr_storage addr{};
			if (!io::Ip::str2addr(observed.ip().c_str(), observed.port(), addr) ||
				addr.ss_family != (sender->ip.find(':') == std::string::npos ? AF_INET : AF_INET6)) return false;
		}
		probe->replies[receiver] = msg;
		if (!probe->replies[1 - receiver]) return false;
		if (probe->path != ProbePath::AdditionalIPv6)
			cxt->winning_lobby_index = static_cast<size_t>(probe - cxt->lobby_probes.data());
		cxt->state = ConnectState::Success;
		return true;
	}

	const boost::uuids::uuid& connect_cxt::get_relay_token()
	{
		if(relay_token)
			return relay_token.value();
		boost::uuids::random_generator generator;
		relay_token = generator();
		return relay_token.value();
	}
	bool connect_cxt::prepare_tls_credential(uint64_t now_ms)
	{
		if (tls_credential) return tls_credential->valid_at(now_ms);
		auto credential = std::make_unique<session_credential>();
		if (!credential->generate(pid[0], pid[1], now_ms)) return false;
		tls_credential = std::move(credential);
		return true;
	}

#ifndef NDEBUG  
	void p2p_model::test_step_cxt()
	{
		connect_cxt cxt;
		cxt.id = merge_id(1,2);
		cxt.pid[0] = 1;
		cxt.pid[1] = 2;
		cxt.ip_list[0].push_back("192.168.1.1");
		cxt.ip_list[0].push_back("192.168.1.2");
		cxt.ip_list[0].push_back("192.168.1.3"); 

		cxt.ip_list[1].push_back("192.168.2.1");
		cxt.ip_list[1].push_back("192.168.2.2");
		cxt.ip_list[1].push_back("192.168.2.3");
		cxt.ip_list[1].push_back("192.168.2.4");
		cxt.state = ConnectState::Ready;

		StepResult res = StepResult::None; 
		do {
			res = next_cxt(cxt);
			printf(" who = %d p1 = %d p2 = %d ty = %hu \n", cxt.stage_1,cxt.stage_2[0],cxt.stage_2[1],static_cast<uint16_t>(cxt.state));
		}while(res != StepResult::End);
	}
#endif
}
