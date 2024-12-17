#include "mqas/tools/model/p2p_model.h"
#include "mqas/io/ip.h"
#include <random>
#include "mqas/core/stream.h"

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

	uint64_t p2p_model::reg_context(uint32_t self, uint32_t oth, const proto::p2p::ClientIpList& oth_ip, 
		std::weak_ptr<mqas::core::IStreamVariant> self_stream, const connect_cxt** out)
	{
		auto client_self = (*this)[self];
		auto client_oth = (*this)[oth];
		*out = nullptr;
		if (client_self == nullptr || client_oth == nullptr)
			return 0;
		auto id = merge_id(self, oth);
		cxt_map.emplace(id, connect_cxt{});
		auto& cxt = cxt_map[id];
		cxt.id = id;
		set_cxt(cxt, first_peer(*client_self, *client_oth), second_peer(*client_self, *client_oth),
			self,oth_ip,std::move(self_stream));
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

	bool p2p_model::init_cxt(connect_cxt& cxt, const peer_data& a, const peer_data& b, const proto::p2p::ClientIpList& a_ip,
		const proto::p2p::ClientIpList& b_ip) const
	{
		cxt.is_same_external = a.ip == b.ip;
		cxt.port_list[0] = a_ip.port();
		cxt.port_list[1] = b_ip.port();
		cxt.pid[0] = a.id;
		cxt.pid[1] = b.id;
		for (int i = 0; i < a_ip.ip_list_size(); ++i)
			if (mqas::io::Ip::valid_ip(a_ip.ip_list().Get(i).c_str(), a_ip.port()))
				cxt.ip_list[0].push_back(a_ip.ip_list().Get(i));
		for (int i = 0; i < b_ip.ip_list_size(); ++i)
			if (mqas::io::Ip::valid_ip(b_ip.ip_list().Get(i).c_str(), b_ip.port()))
				cxt.ip_list[1].push_back(b_ip.ip_list().Get(i));
		cxt.state = ConnectState::Idle;
		cxt.stage_1 = cxt.stage_2[0] = cxt.stage_2[1] =  -1;
		cxt.tag = 0;
		return true;
	}

	bool p2p_model::set_cxt(connect_cxt& cxt, const peer_data& a, const peer_data& b, uint32_t id, const proto::p2p::ClientIpList& oth_ip,
		std::weak_ptr<mqas::core::IStreamVariant> self_stream) const
	{
		cxt.is_same_external = a.ip == b.ip;
		cxt.pid[0] = a.id;
		cxt.pid[1] = b.id;
		const auto idx = a.id == id ? 0 : 1;
		const auto oth_idx = idx == 0 ? 1 : 0;
		cxt.port_list[oth_idx] = oth_ip.port();
		for (int i = 0; i < oth_ip.ip_list_size(); ++i)
			if(mqas::io::Ip::valid_ip(oth_ip.ip_list().Get(i).c_str(), oth_ip.port()))
				cxt.ip_list[oth_idx].push_back(oth_ip.ip_list().Get(i));
		cxt.stage_1 = cxt.stage_2[idx] = -1;
		cxt.tag = 0;
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
			return { res,generate_connect_data(*cxt,self)};
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
			return { StepResult::Success,generate_connect_data(*cxt,self) };
		case ConnectState::Success:
		case ConnectState::Wait:
			return {StepResult::End,{}};
		}
		return { StepResult::None,{} };
	}
	//int8_t stage_1;//who active
	//std::array<int8_t, 2> stage_2;//ip index
	//int8_t tag;// success count
	StepResult p2p_model::next_cxt(connect_cxt& cxt) const
	{
		const int size = cxt.pid.size();

		switch (cxt.state)
		{
		case ConnectState::Idle:
			return StepResult::None;
		case ConnectState::Ready:
		case ConnectState::ChangeToExternal:
			cxt.stage_1 = 0;
			cxt.stage_2[0] = cxt.stage_2[1] = 0;
			cxt.state = cxt.state == ConnectState::Ready ? ConnectState::TryInternal : ConnectState::TryExternal;
			return StepResult::Success;
		case ConnectState::TryInternal:
		case ConnectState::TryExternal:
		{
			int8_t& a = cxt.stage_2[0];
			int8_t& b = cxt.stage_2[1];
			int8_t as = ConnectState::TryExternal == cxt.state ? 1 : cxt.ip_list[0].size();
			int8_t bs = ConnectState::TryExternal == cxt.state ? 1 : cxt.ip_list[1].size();

			if (a + 1 >= as && b + 1 > bs)
			{
				if (cxt.state == ConnectState::TryInternal)
				{
					cxt.state = ConnectState::ChangeToExternal;
					return next_cxt(cxt);
				}
				else {
					cxt.state = ConnectState::Wait;
					return next_cxt(cxt);
				}
			}
			++a;
			++b;
			if (a + 1 >= as && b + 1 > bs)
				return next_cxt(cxt);
			generate_verify_code(cxt.verify_code);
			return StepResult::Success;
		}
		case ConnectState::Success:
		case ConnectState::Wait:
			return StepResult::End;
			break;
		}
		return StepResult::None;
	}

	std::optional<proto::p2p::NotifyConnectPeerData> p2p_model::generate_connect_data(const connect_cxt& cxt, uint32_t self) const
	{
		if(!(cxt.state == ConnectState::TryInternal || cxt.state == ConnectState::TryExternal))
			return {};
		auto oth = other(cxt.pid, self);
		auto idx_oth = oth_idx(cxt.pid,self);
		auto self_data = (*this)[self];
		auto oth_data = (*this)[oth];
		auto ip_idx = -1;
		if (idx_oth >= cxt.stage_2.size())
			return {};
		ip_idx = cxt.stage_2[idx_oth];
		if(self_data == nullptr || oth_data == nullptr)
			return {};
		proto::p2p::NotifyConnectPeerData data;
		data.set_peer_id(oth);
		proto::p2p::ConnectPeerData* d = data.mutable_connect_data();
		d->set_ip( cxt.state == ConnectState::TryInternal ? cxt.ip_list[idx_oth][ip_idx] : oth_data->ip);
		d->set_port( cxt.state == ConnectState::TryInternal ? cxt.port_list[idx_oth] : oth_data->port);
		d->set_verify_code( cxt.verify_code[idx_oth]);
		d->set_send_times(3);
		d->set_send_delay(200);
		
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

	bool p2p_model::submit_verify_code(uint64_t id, uint32_t who, uint32_t code)
	{
		auto cxt = get_context(id);
		if (!(cxt->state == ConnectState::TryInternal || cxt->state == ConnectState::TryExternal))
			return false;
		auto idx_oth = oth_idx(cxt->pid, who);
		if (cxt->verify_code[idx_oth] == code)
		{
			cxt->tag |= (1 << self_idx(cxt->pid, who));
		}
		return *cxt;
	}

	bool connect_cxt::is_receive(uint32_t id) const
	{
		auto idx = self_idx(pid, id);
		return ((tag >> idx) & 1) > 0;
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