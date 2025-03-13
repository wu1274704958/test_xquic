#pragma once
#include <string>
#include <numeric>
#include <map>
#include <functional>
#include <unordered_set>
#include <array>
#include "mqas/tools/proto/p2p.pb.h"
#include <uv.h>
#include <optional>
#include <boost/uuid/uuid.hpp>

namespace mqas::core {
	class MQAS_EXTERN IStreamVariant;
}
namespace mqas::tools::p2p {

	template<typename I, typename T>
	T& min(I a, I b, T& av, T& bv)
	{
		auto min = std::min(a, b);
		return min == a ? av : bv;
	}
	template<typename I, typename T>
	T& max(I a, I b, T& av, T& bv)
	{
		auto max = std::max(a, b);
		return max == a ? av : bv;
	}
	template<typename I>
	I other(std::array<I, 2> arr, I x)
	{
		return x == arr[0] ? arr[1] : arr[0];
	}
	template<typename I>
	I oth_idx(std::array<I, 2> arr, I x)
	{
		return x == arr[0] ? 1 : 0;
	}
	template<typename I>
	I self_idx(std::array<I, 2> arr, I x)
	{
		return x == arr[0] ? 0 : 1;
	}

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
		std::weak_ptr<mqas::core::IStreamVariant> stream;

		peer_data(uint32_t id,const std::string& name, const std::string& ip,
			uint16_t port, std::weak_ptr<mqas::core::IStreamVariant> stream) : id(id), name(name), state(PeerState::Idle),
			ip(ip),port(port), connect_success_count(0),stream(stream)
		{}
		peer_data(const peer_data&) = default;
		peer_data(peer_data&&) = default;
	};

	enum class ConnectState : uint16_t
	{
		Idle = 0,
		Ready,
		TryInternal,
		ChangeToExternal,
		TryExternal,
		Success,
		Wait
	};

	enum class StepResult : uint32_t
	{
		None = 0,
		Success = 1,
		End = 4
	};

	struct connect_cxt {
		static constexpr int16_t ExternalIpIndex = 9999;
		uint64_t id;
		std::array<uint32_t, 2> pid;
		std::array<std::vector<std::string>, 2> ip_list;
		std::array<uint16_t, 2> port_list;
		ConnectState state;
		int8_t stage_1;//who active
		std::array<int8_t, 2> stage_2;
		std::array<int16_t, 2> tag;// success tag
		bool is_same_external;
		std::array<uint32_t,2> verify_code;
		std::array<std::weak_ptr<mqas::core::IStreamVariant>, 2> stream;
		std::array<std::unordered_map<int16_t,std::shared_ptr<proto::p2p::ReqSubmitRecvPeerKeyCode>>,2> submit_code_map;
		private:
		std::optional<boost::uuids::uuid> token;
		//func
		public:
		connect_cxt() : port_list({0,0}) {}
		inline operator bool() const { return tag[0] >= 0 && tag[1] >= 0; }
		bool is_receive(uint32_t id) const;
		const boost::uuids::uuid& get_token();
	};

	/*template<typename T, typename = std::void_t<>>
	struct has_static_function_next_step_connect_context : std::false_type {};

	template<typename T>
	struct has_static_function_next_step_connect_context<T, std::void_t<decltype(T::next_step_connect_context(std::declval<connect_cxt&>()))>>
		: std::is_same<decltype(T::next_step_connect_context(std::declval<connect_cxt&>())), StepResult > {};


	struct def_step_connect_context {
		static StepResult next_step_connect_context(connect_cxt&);
	};

	template<typename T = def_step_connect_context>
	requires requires{
		requires has_static_function_next_step_connect_context<T>::value;
	}*/
	class p2p_model
	{

	public:
		//lobby
		uint32_t registe_client(const std::string& name, const std::string& ip, uint16_t port,std::weak_ptr<mqas::core::IStreamVariant> stream);
		bool unregiste_client(uint32_t id);
		void visit_client(std::function<void(const peer_data&)> f) const;
		const peer_data* operator[](uint32_t id) const;
		template <typename T>
		bool visit_client_stream(uint32_t id,std::function<bool(std::shared_ptr<T>)> f) const
		{
			auto client = (*this)[id];
			if(client == nullptr)
				return false;
			auto shared_ptr = client->stream.lock();
			if(client == nullptr || !shared_ptr)
				return false;
			auto ptr = std::dynamic_pointer_cast<T>(shared_ptr);
			if (!ptr)
				return false;
			return f(ptr);
		}
		//helper
		template <typename T>
		std::shared_ptr<T> get_helper_stream(uint64_t mid,uint32_t id) const
		{
			const auto cxt = get_context_const(mid);
			if(cxt == nullptr)
				return nullptr;
			const auto idx = self_idx(cxt->pid,id);
			auto ptr = cxt->stream[idx];
			auto shared_ptr = ptr.lock();
			if (!shared_ptr) return nullptr;
			return std::dynamic_pointer_cast<T>(shared_ptr);
		}
		uint64_t reg_context(uint32_t self, uint32_t oth,const proto::p2p::ClientIpList& self_ip, std::weak_ptr<mqas::core::IStreamVariant> stream,const connect_cxt** out);
		/// <summary>
		/// 
		/// </summary>
		/// <param name="self">self id</param>
		/// <param name="oth">other id</param>
		/// <returns>-1:context not exist,0:the two peers have been unregistered and context removed,1:other peer still</returns>
		int unreg_context(uint32_t self, uint32_t oth);
		bool exist_context_peer(uint64_t mid, uint32_t id) const;
		
		void clear_context(uint64_t mid);
		const connect_cxt* get_context_const(uint64_t id) const;
		const connect_cxt* get_context_const(uint32_t a, uint32_t b) const;
		std::pair<StepResult, std::optional<proto::p2p::NotifyConnectPeerData>> next_cxt(uint64_t id, uint32_t self);
		std::pair<StepResult, std::optional<proto::p2p::NotifyConnectPeerData>> current_cxt(uint64_t id, uint32_t self) const;
		std::optional<proto::p2p::NotifyConnectPeerData> generate_connect_data(const connect_cxt& cxt, uint32_t self) const;
		bool submit_verify_code(uint64_t merge_id, uint32_t id, const std::shared_ptr<proto::p2p::ReqSubmitRecvPeerKeyCode>& msg);
		#ifndef NDEBUG  
		void test_step_cxt();
		#endif
		static uint64_t merge_id(uint32_t a, uint32_t b);

	protected:
		//lobby
		inline void update_min_id() { min_id = client_map.empty() ? 0 : (client_map.begin()->second.id); }
		inline void update_max_id() { max_id = client_map.empty() ? 0 : ((--client_map.end())->second.id); }
		uint32_t next_id();
		//helper
		bool init_cxt(connect_cxt& cxt,const peer_data& a, const peer_data& b, const proto::p2p::ClientIpList& a_ip,
			const proto::p2p::ClientIpList& b_ip) const;
		bool set_cxt(connect_cxt& cxt, const peer_data& a, const peer_data& b,uint32_t id,const proto::p2p::ClientIpList& self_ip,
			std::weak_ptr<mqas::core::IStreamVariant> self_stream) const;
		StepResult next_cxt(connect_cxt& cxt) const;
		void generate_verify_code(std::array<uint32_t, 2>& cxt) const;
		
		const peer_data& first_peer(const peer_data& a, const peer_data& b) const;
		const peer_data& second_peer(const peer_data& a, const peer_data& b) const;
		connect_cxt* get_context(uint64_t id);
		connect_cxt* get_context(uint32_t a, uint32_t b);

	protected:
		std::map<uint32_t, peer_data> client_map;
		std::map<uint64_t, connect_cxt> cxt_map;
		
	private:
		uint32_t min_id = 0;
		uint32_t max_id = 0;
	};

}