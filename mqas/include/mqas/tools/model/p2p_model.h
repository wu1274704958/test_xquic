#pragma once
#include <string>
#include <numeric>
#include <map>
#include <functional>
#include <unordered_set>

namespace mqas::tools::p2p {

	enum class PeerState : uint32_t
	{
		Idle = 0,
		Connecting = 1,
	};

	struct peer_data
	{
		uint32_t id;
		std::string name;
		PeerState state;
		std::string ip;
		uint16_t port;
		uint16_t connect_success_count;
		std::unordered_set<uint32_t> connected_set;

		peer_data(uint32_t id,const std::string& name, const std::string& ip,
			uint16_t port) : id(id), name(name), state(PeerState::Idle),
			ip(ip),port(port), connect_success_count(0)
		{}
		peer_data(const peer_data&) = default;
		peer_data(peer_data&&) = default;
	};

	class p2p_model
	{

	public:
		uint32_t registe_client(const std::string& name, const std::string& ip, uint16_t port);
		bool unregiste_client(uint32_t id);
		void visit_client(std::function<void(const peer_data&)> f) const;
		const peer_data* operator[](uint32_t id) const;

	protected:
		inline void update_min_id() { min_id = client_map.empty() ? 0 : (client_map.begin()->second.id); }
		inline void update_max_id() { max_id = client_map.empty() ? 0 : ((--client_map.end())->second.id); }
		uint32_t next_id();

	protected:
		std::map<uint32_t, peer_data> client_map;
		
	private:
		uint32_t min_id = 0;
		uint32_t max_id = 0;
	};

}