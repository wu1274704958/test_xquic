#include "mqas/tools/model/p2p_model.h"

uint32_t mqas::tools::p2p::p2p_model::registe_client(const std::string& name, const std::string& ip, uint16_t port)
{
	auto id = next_id();
	if (id == 0)
		return 0;
	client_map.insert({ id, peer_data(id, name, ip, port) });
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
	if(client_map.find(id) != client_map.end())
		return &client_map.at(id);
	return nullptr;
}

uint32_t mqas::tools::p2p::p2p_model::next_id()
{
	if (min_id - 1 > 0)
		return min_id - 1;
	if (max_id + 1 == std::numeric_limits<uint32_t>::max())
		return 0;
	return max_id + 1;
}

