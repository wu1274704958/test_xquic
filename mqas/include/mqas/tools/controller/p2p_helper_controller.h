#pragma once
#include <numeric>
#include "mqas/tools/model/p2p_model.h"
#include "mqas/io/timer.h"
#include <functional>


namespace mqas::tools::controller {
	class p2p_helper_controller {
	public:
		p2p_helper_controller(uint32_t a,uint32_t b,io::Context* io_cxt);
		operator bool() const;
		bool ready() const;
		bool is_start() const;
		bool init(uint32_t a, uint32_t b);
		bool peer_exist(uint32_t a) const;
		void start();
		void submit_verify_code(uint64_t id, uint32_t who, uint32_t code);
		void on_verify_success() const;
		void on_timeout() const;
		void register_event(uint32_t id, std::function<void(const proto::p2p::NotifyConnectPeerData&)> notify_connect,
			std::function<void(const proto::p2p::NotifyConnectResult&)> notify_result);
		void stop(std::optional<std::string> reason);
	protected:
		void stop_step();
		void on_step(io::Timer*);
		void notify_success(uint32_t id) const;
		void notify_failed(uint32_t id,const std::optional<std::string>& reason) const;
		void set_reason(const std::string&);
	protected:
		const p2p::connect_cxt* _cxt;
		io::Context* _io_cxt;
		std::array<const p2p::peer_data*, 2> _peer_list;
		std::array<std::function<void(const proto::p2p::NotifyConnectPeerData&)>, 2> _notify_connect_signal;
		std::array<std::function<void(const proto::p2p::NotifyConnectResult&)>, 2> _notify_connect_result;
		std::shared_ptr<io::Timer> _timer;
		bool _is_start : 1;
		std::optional<std::string> _reason;
	};
}

