#include "mqas/tools/controller/p2p_helper_controller.h"
#include "mqas/comm/locator.h"
#include <format>
#include <boost/uuid/uuid.hpp>
#include <chrono>
#include <easylogging++.h>
#include <openssl/crypto.h>

namespace mqas::tools::controller {
	static constexpr uint64_t probe_window_ms = 1000;
	static constexpr uint64_t ipv6_start_delay_ms = 250;

	p2p_helper_controller::p2p_helper_controller(uint32_t a, uint32_t b,io::Context* io_cxt,std::optional<toml::value> config) : _is_start(false), _cxt(nullptr),
		_io_cxt(io_cxt),_config(config)
	{
		init(a, b);
	}

	p2p_helper_controller::operator bool() const
	{
		return _cxt != nullptr && _cxt->state >= p2p::ConnectState::Ready;
	}

	bool p2p_helper_controller::ready() const
	{
		return *this && _notify_connect_result[0] && _notify_connect_result[1] &&
			_notify_connect_signal[0] && _notify_connect_signal[1];
	}

	bool p2p_helper_controller::is_start() const
	{
		return _timer && _is_start;
	}

	bool p2p_helper_controller::init(uint32_t a, uint32_t b)
	{
		auto model = comm::locator::inst()->get<p2p::p2p_model>();
		_cxt = model ? model->get().get_context_const(a, b) : nullptr;
		return *this;
	}

	void p2p_helper_controller::submit_verify_code(uint64_t merge_id,uint32_t id,const std::shared_ptr<proto::p2p::ReqSubmitRecvPeerKeyCode>& msg)
	{
		auto model = comm::locator::inst()->get<p2p::p2p_model>();
		if (!is_start() || !model)
			return;
		if (model.value().get().submit_verify_code(merge_id,id,msg))
		{
			if (is_start())
				stop_step();
			on_verify_success();
		}
	}

	static const char* address_family(const std::string& ip) {
		return ip.empty() ? "unknown" : ip.find(':') == std::string::npos ? "ipv4" : "ipv6";
	}

	static unsigned confirmations(const p2p::ProbeAttempt& probe) {
		return unsigned(bool(probe.replies[0])) + unsigned(bool(probe.replies[1]));
	}
	void p2p_helper_controller::log_path_result(const char* result) const
	{
		unsigned best_lobby = 0;
		for (const auto& attempt : _cxt->lobby_probes)
			best_lobby = std::max(best_lobby, confirmations(attempt));
		auto model = comm::locator::inst()->get<p2p::p2p_model>();
		auto family = [&](uint32_t id) {
			const auto* peer = model ? model->get()[id] : nullptr;
			return peer ? address_family(peer->ip) : "unknown";
		};
		LOG(INFO) << "p2p_path_selection first_family=" << family(_cxt->pid[0])
			<< " second_family=" << family(_cxt->pid[1])
			<< " elapsed_ms=" << std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - _probe_start).count()
			<< " lobby_confirmations=" << best_lobby
			<< " ipv6_confirmations=" << (_cxt->ipv6_probe ? confirmations(*_cxt->ipv6_probe) : 0)
			<< " lobby_pairs=" << _cxt->lobby_probes.size()
			<< " ipv6_started=" << _cxt->ipv6_probe.has_value()
			<< " additional_ipv6=" << (_cxt->winner() && _cxt->winner()->path == p2p::ProbePath::AdditionalIPv6) << " result=" << result;
	}

	void p2p_helper_controller::on_verify_success() const
	{
		if (!ready())
			return;
		const auto now = std::chrono::duration_cast<std::chrono::milliseconds>(
			std::chrono::system_clock::now().time_since_epoch()).count();
		if (now < 0 || !const_cast<p2p::connect_cxt*>(_cxt)->prepare_tls_credential(static_cast<uint64_t>(now)))
		{
			notify_failed(_cxt->pid[0], std::string("Unable to create session credential"));
			notify_failed(_cxt->pid[1], std::string("Unable to create session credential"));
			return;
		}
		if (auto model = comm::locator::inst()->get<p2p::p2p_model>()) {
			const auto* credential = _cxt->get_tls_credential();
			const auto server = _cxt->select_server();
			const auto client = p2p::other(_cxt->pid, server);
			const auto& id = credential->negotiation_id();
			model->get().track_ready(std::string(reinterpret_cast<const char*>(id.data()), id.size()), server, client, credential->expires_at_ms());
		}
		log_path_result(has_relay_config() && always_use_relay() ? "relay_selected" : "direct_selected");
		notify_success(_cxt->pid[0]);
		notify_success(_cxt->pid[1]);
	}

	void p2p_helper_controller::on_timeout() const
	{
		if (!ready())
			return;
		log_path_result(has_relay_config() ? "relay_fallback" : "failed");
		notify_failed(_cxt->pid[0],_reason);
		notify_failed(_cxt->pid[1],_reason);
	}

	void p2p_helper_controller::start() 
	{
		if (!ready() || is_start())
			return;
		if (!_timer)
			_timer = _io_cxt->make_shared<io::Timer>();
		_probe_start = std::chrono::steady_clock::now();
		_is_start = true;
		_timer->start(std::bind_front(&p2p_helper_controller::on_step, this), 0, probe_window_ms);
		if (_cxt->additional_ipv6[0] && _cxt->additional_ipv6[1]) {
			_ipv6_timer = _io_cxt->make_shared<io::Timer>();
			_ipv6_timer->start(std::bind_front(&p2p_helper_controller::start_ipv6, this), ipv6_start_delay_ms, 0);
		}
	}

	void p2p_helper_controller::stop_step()
	{
		if (is_start())
		{
			_is_start = false;
			_timer->stop();
			if (_ipv6_timer) _ipv6_timer->stop();
		}
	}

	void p2p_helper_controller::start_ipv6(io::Timer*)
	{
		auto model = comm::locator::inst()->get<p2p::p2p_model>();
		if (!is_start() || !model || !peers_available() || !model->get().start_ipv6_probe(_cxt->id)) return;
		// If the external pair is already running, the next lobby tick ends negotiation.
		// Give the later IPv6 probes their full reply window as well.
		if (_cxt->state == p2p::ConnectState::TryExternal)
			_timer->start(std::bind_front(&p2p_helper_controller::on_step, this), probe_window_ms, probe_window_ms);
		for (size_t i = 0; i < 2; ++i) {
			auto msg = model->get().generate_connect_data(*_cxt, _cxt->pid[i], *_cxt->ipv6_probe);
			if (msg) _notify_connect_signal[i](*msg);
		}
	}
	bool p2p_helper_controller::peers_available()
	{
		for (auto id : _cxt->pid) {
			if (!peer_exist(id)) {
				stop(std::format("peer {} left", id));
				return false;
			}
		}
		return true;
	}
	void p2p_helper_controller::on_step(io::Timer* t)
	{
		if (!ready())
			return;
		auto model = comm::locator::inst()->get<p2p::p2p_model>();
		if (!model)
			return;
		auto res = model.value().get().next_cxt(_cxt->id, _cxt->pid[0]);
		switch (res.first)
		{
		case p2p::StepResult::End:
			set_reason("timeout");
			on_timeout();
			stop_step();
			return;
		case p2p::StepResult::None:
			set_reason("unknow");
			on_timeout();
			stop_step();
			return;
		default:
			break;
		}
		if (!peers_available()) return;
		if (res.second) _notify_connect_signal[0](*res.second);
		res = model->get().current_cxt(_cxt->id, _cxt->pid[1]);
		if (res.second) _notify_connect_signal[1](*res.second);
	}

	void p2p_helper_controller::register_event(uint32_t id, std::function<void(const proto::p2p::NotifyConnectPeerData&)> notify_connect,
		std::function<void(const proto::p2p::NotifyConnectResult&)> notify_result)
	{
		if (!*this)
			return;
		const auto idx = p2p::self_idx(_cxt->pid, id);
		_notify_connect_result[idx] = notify_result;
		_notify_connect_signal[idx] = notify_connect;
	}

	void p2p_helper_controller::set_current_address(uint32_t id,proto::p2p::Address* addr) const
	{
		if (!*this)
			return;
		auto model = comm::locator::inst()->get<p2p::p2p_model>();
		if (!model)
			return;
		const auto idx = p2p::self_idx(_cxt->pid, id);
		switch (_cxt->winner()->path)
		{
		case p2p::ProbePath::AdditionalIPv6:
			*addr = *_cxt->additional_ipv6[idx];
			break;
		case p2p::ProbePath::LobbyExternal:
		{
			const auto peer = model->get()[id];
			if (peer == nullptr)
				return;
			addr->set_ip(peer->ip);
			addr->set_port(peer->port);
			break;
		}
		case p2p::ProbePath::LobbyInternal:
		{
			addr->set_port(_cxt->port_list[idx]);
			const auto& list = _cxt->ip_list[idx];
			addr->set_ip(list[_cxt->winner()->index[idx]]);
			break;
		}
		default:
			break;
		}
	}
	
	void p2p_helper_controller::set_external_address(uint32_t id,proto::p2p::Address* addr) const
	{
		if (!*this)
			return;
		auto model = comm::locator::inst()->get<p2p::p2p_model>();
		if (!model)
			return;
		const auto peer = model->get()[id];
		if (peer == nullptr)
			return;
		addr->set_ip(peer->ip);
		addr->set_port(peer->port);
	}

	void p2p_helper_controller::notify_success(uint32_t id) const
	{
		if (!peer_exist(id))
			return;
		const auto idx = p2p::self_idx(_cxt->pid, id);
		mqas::tools::proto::p2p::NotifyConnectResult msg;
		msg.set_peer_id(tools::p2p::other(_cxt->pid, id));
		msg.set_ret(mqas::tools::proto::p2p::RetCode::ok);
		msg.set_is_server(id == _cxt->select_server());
		const auto* credential = _cxt->get_tls_credential();
		if (!credential) return;
		msg.set_psk_identity(credential->identity().data(), credential->identity().size());
		msg.set_psk_secret(credential->secret().data(), credential->secret().size());
		msg.set_psk_issued_at_ms(credential->issued_at_ms());
		msg.set_psk_expires_at_ms(credential->expires_at_ms());
		msg.set_negotiation_id(credential->negotiation_id().data(), credential->negotiation_id().size());
		if (has_relay_config() && always_use_relay())
			append_relay(id,msg);
		else{
			auto address = msg.mutable_address();
			auto peer_addr = msg.mutable_peer_addr();
			if (_cxt->winner() && _cxt->winner()->path == p2p::ProbePath::AdditionalIPv6) msg.set_use_non_lobby_socket(true);
			set_current_address(id,address);
			*peer_addr = _cxt->winner()->replies[idx]->peer_addr();
		}
		_notify_connect_result[idx](msg);
		auto* secret = msg.mutable_psk_secret();
		if (!secret->empty()) OPENSSL_cleanse(secret->data(), secret->size());
	}
	void p2p_helper_controller::notify_failed(uint32_t id, const std::optional<std::string>& reason) const
	{
		if (!peer_exist(id))
			return;
		const auto idx = p2p::self_idx(_cxt->pid, id);
		mqas::tools::proto::p2p::NotifyConnectResult msg;
		msg.set_peer_id(tools::p2p::other(_cxt->pid, id));
		msg.set_ret(mqas::tools::proto::p2p::RetCode::failed);
		if (reason)
			msg.set_reason(reason.value());
		if (has_relay_config())
		{
			append_relay(id,msg);
			msg.set_is_server(id == _cxt->select_server());
		}
		_notify_connect_result[idx](msg);
	}
	bool p2p_helper_controller::peer_exist(uint32_t id) const
	{
		if (!ready())
			return false;
		auto model = comm::locator::inst()->get<p2p::p2p_model>();
		if (!model || model->get().get_context_const(_cxt->id) == nullptr)
			return false;
		return model.value().get()[id] != nullptr && model->get().exist_context_peer(_cxt->id,id);
	}
	void p2p_helper_controller::set_reason(const std::string& s)
	{
		_reason = { std::move(s) };
	}
	void p2p_helper_controller::stop(std::optional<std::string> reason)
	{
		if (!ready() || !is_start())
			return;
		stop_step();
		if(reason)
			set_reason(reason.value());
		on_timeout();
	}

	bool p2p_helper_controller::has_relay_config() const
	{
		return _config && _config->contains("relay") && _config->at("relay").contains("ip") && _config->at("relay").contains("port");
	}
	bool p2p_helper_controller::always_use_relay() const
	{
		return _config && toml::find_or<bool>(*_config,"relay","always",false);
	}
	void p2p_helper_controller::append_relay(uint32_t id,proto::p2p::NotifyConnectResult& msg) const
	{
		auto relay_addr = msg.mutable_relay_addr();
		relay_addr->set_ip(toml::find<std::string>(*_config,"relay","ip"));
		relay_addr->set_port(toml::find<uint16_t>(*_config,"relay","port"));

		auto address = msg.mutable_address();
		auto peer_addr = msg.mutable_peer_addr();

		const auto oth_id = p2p::other(_cxt->pid, id);

		set_external_address(id,address);
		set_external_address(oth_id, peer_addr);

		msg.set_use_relay(true);

		auto cxt = const_cast<p2p::connect_cxt*>(_cxt);
		
		auto& token = cxt->get_relay_token();

		msg.set_relay_token((const char*)&token.data,token.size());
	}
}
