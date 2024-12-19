#include "mqas/tools/controller/p2p_helper_controller.h"
#include "mqas/comm/locator.h"
#include <format>

namespace mqas::tools::controller {
	p2p_helper_controller::p2p_helper_controller(uint32_t a, uint32_t b,io::Context* io_cxt) : _is_start(false), _cxt(nullptr),
		_io_cxt(io_cxt)
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
		if (model)
		{
			_cxt = model.value().get().get_context_const(a, b);
			if ((bool)(*this))
			{
				_peer_list[0] = model.value().get()[_cxt->pid[0]];
				_peer_list[1] = model.value().get()[_cxt->pid[1]];
			}
		}
		else
			_cxt = nullptr;
		return *this;
	}

	void p2p_helper_controller::submit_verify_code(uint64_t id, uint32_t who, uint32_t code, uint16_t ip_index)
	{
		auto model = comm::locator::inst()->get<p2p::p2p_model>();
		if (!*this && !model)
			return;
		if (model.value().get().submit_verify_code(id, who, code,ip_index))
		{
			if (is_start())
				stop_step();
			on_verify_success();
		}
	}

	void p2p_helper_controller::on_verify_success() const
	{
		if (!ready())
			return;
		notify_success(_peer_list[0]->id);
		notify_success(_peer_list[1]->id);
	}

	void p2p_helper_controller::on_timeout() const
	{
		if (!ready())
			return;
		notify_failed(_peer_list[0]->id,_reason);
		notify_failed(_peer_list[1]->id,_reason);
	}

	void p2p_helper_controller::start() 
	{
		if (!ready() || is_start())
			return;
		if (!_timer)
			_timer = _io_cxt->make_shared<io::Timer>();
		_is_start = true;
		_timer->start(std::bind(&p2p_helper_controller::on_step, this, std::placeholders::_1), 0, 1000);
	}

	void p2p_helper_controller::stop_step()
	{
		if (is_start())
		{
			_is_start = false;
			_timer->stop();
		}
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
		if (!peer_exist(_peer_list[0]->id))
		{
			set_reason(std::format("peer {} leaved", _peer_list[0]->id));
			on_timeout();
			stop_step();
		}
		else if(res.second)
			_notify_connect_signal[0](*res.second);
		res = model.value().get().current_cxt(_cxt->id, _cxt->pid[1]);
		assert(res.first == p2p::StepResult::Success);
		if (!peer_exist(_peer_list[1]->id))
		{
			set_reason(std::format("peer {} leaved", _peer_list[1]->id));
			on_timeout();
			stop_step();
		}else if(res.second)
			_notify_connect_signal[1](*res.second);
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
		if (!*this && !*_cxt)
			return;
		auto model = comm::locator::inst()->get<p2p::p2p_model>();
		if (!model)
			return;
		const auto idx = p2p::oth_idx(_cxt->pid, id);
		switch (_cxt->state)
		{
		case p2p::ConnectState::TryExternal:
		{
			const auto peer = model->get()[id];
			if (peer == nullptr)
				return;
			addr->set_ip(peer->ip);
			addr->set_port(peer->port);
			break;
		}
		case p2p::ConnectState::TryInternal:
		{
			addr->set_port(_cxt->port_list[idx]);
			const auto& list = _cxt->ip_list[idx];
			auto ip_idx = _cxt->tag[idx];
			if (ip_idx >= list.size())
				ip_idx = list.size() - 1;
			addr->set_ip(list[ip_idx]);
			break;
		}
		default:
			break;
		}
	}

	void p2p_helper_controller::notify_success(uint32_t id) const
	{
		if (!peer_exist(id))
			return;
		const auto idx = p2p::self_idx(_cxt->pid, id);
		const auto oth_id = p2p::other(_cxt->pid, id);
		mqas::tools::proto::p2p::NotifyConnectResult msg;
		msg.set_peer_id(tools::p2p::other(_cxt->pid, id));
		msg.set_ret(mqas::tools::proto::p2p::RetCode::ok);
		msg.set_is_server(idx == 0);
		auto address = msg.mutable_address();
		set_current_address(oth_id,address);
		_notify_connect_result[idx](msg);
	}
	void p2p_helper_controller::notify_failed(uint32_t id, const std::optional<std::string>& reason) const
	{
		if (!peer_exist(id))
			return;
		const auto idx = p2p::self_idx(_cxt->pid, id);
		const auto oth_id = p2p::other(_cxt->pid, id);
		mqas::tools::proto::p2p::NotifyConnectResult msg;
		msg.set_peer_id(tools::p2p::other(_cxt->pid, id));
		msg.set_ret(mqas::tools::proto::p2p::RetCode::failed);
		if (reason)
			msg.set_reason(reason.value());
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
}