#include "mqas/tools/stream/p2p_helper.h"
#include "mqas/tools/model/p2p_model.h"
#include "mqas/comm/locator.h"
#include "mqas/tools/controller/p2p_helper_controller.h"
#include "mqas/tools/stream/p2p_lobby.h"

using namespace mqas::comm;

namespace mqas::tools::p2p {


    P2PHelperStream::P2PHelperStream()
    {
        
    }

    P2PHelperStream::~P2PHelperStream()
    {
        stop_check_timeout();
        on_leave();
    }

    void P2PHelperStream::stop_check_timeout()
    {
        if (_timeout_timer)
            _timeout_timer->stop();
        _timeout_timer.reset();
    }

    void P2PHelperStream::on_close()
    {
        on_leave();
    }

    void P2PHelperStream::on_leave()
    {
        if (_merge_id > 0 && _other_id > 0 && _self != nullptr)
        {
            auto model = locator::inst()->get<p2p::p2p_model>();
            if (model)
            {
                const auto res = model->get().unreg_context(_self->id, _other_id);
                if (res == 0)
                    locator::inst()->remove<tools::controller::p2p_helper_controller>((size_t)_merge_id);
                else if(!_notified_result && res == 1)
                    stop(std::format("peer {} leave!", _self->id));
            }
        }
    }

    void P2PHelperStream::on_timeout(io::Timer* t)
    {
        if (!_notified_result && _merge_id > 0 && _other_id > 0 && _self != nullptr)
        {
            stop(std::format("wating peer {} timeout!", _other_id));
        }
    }

    P2PHelperStream::operator bool() const
    {
        return _self != nullptr;
    }

    core::StreamVariantErrcode P2PHelperStream::on_change_msg_s(const std::shared_ptr<proto::p2p::ReqConnectPeer>& msg, std::vector<uint8_t>& ret)
    {
        const auto res = on_peer_connect(msg->peer_id(),msg->ip_list());
        proto::p2p::RespondConnectPeer ret_msg;
        ret_msg.set_peer_id(msg->peer_id());
        ret_msg.set_ret(res == core::StreamVariantErrcode::ok ? proto::p2p::RetCode::ok : proto::p2p::RetCode::failed);
        core::ProtoBufMsg::write_msg<RespondConnectPeerPair>(ret, ret_msg);
        return res;
    }


    core::StreamVariantErrcode P2PHelperStream::on_change_msg_s(const std::shared_ptr<proto::p2p::ReqRespondPeerReqConnect>& msg,
        std::vector<uint8_t>& ret)
    {
		auto model = comm::locator::inst()->get<p2p_model>();
		if (!model)return core::StreamVariantErrcode::failed;

		const auto res = on_peer_connect(msg->peer_id(), msg->ip_list());
		proto::p2p::RetCode ret_code = res == core::StreamVariantErrcode::ok ? proto::p2p::RetCode::ok : proto::p2p::RetCode::failed;
		//send RespondConnectPeer to peer
		if (ret_code == proto::p2p::RetCode::ok)
		{
			if (!model.value().get().visit_client_stream<P2PLobbyStream>(msg->peer_id(), [msg, this](std::shared_ptr<P2PLobbyStream> ptr)->bool {
				proto::p2p::RespondConnectPeer msg;
				msg.set_peer_id(_self->id);
				msg.set_ret(proto::p2p::RetCode::ok);
				ptr->send_lazy<RespondConnectPeerPair>(msg);
				return true;
				}))
				ret_code = proto::p2p::not_exists;
		}
		proto::p2p::RespondConnectPeer ret_msg;
		ret_msg.set_peer_id(msg->peer_id());
		ret_msg.set_ret(ret_code);
		core::ProtoBufMsg::write_msg<RespondConnectPeerPair>(ret, ret_msg);
        return res;
    }

    core::StreamVariantErrcode P2PHelperStream::on_peer_connect(uint32_t id, const proto::p2p::ClientIpList & ip)
    {
        _self = connect_cxt_->get_cxt<p2p::peer_data>();
        auto locator = locator::inst();
        auto model = locator->get<p2p::p2p_model>();
        if (!*this || !model)
            return core::StreamVariantErrcode::failed;

        const p2p::connect_cxt* cxt;
        _merge_id = model.value().get().reg_context(_self->id, id, ip,this->weak_from_this(), &cxt);
        _other_id = id;
        if (cxt->state == p2p::ConnectState::Ready)
        {
            std::optional<toml::value> relay_conf = {};
            auto conf = connect_cxt_->engine_cxt_->engine.lock()->get_config();
            if(conf->contains("p2p"))
                relay_conf = conf->operator[]("p2p");
            locator->deposit_cxt<tools::controller::p2p_helper_controller>((size_t)_merge_id, _self->id, id, &connect_cxt_->engine_cxt_->io_cxt,relay_conf);
            auto controller = locator->get<tools::controller::p2p_helper_controller>((size_t)_merge_id);
            setup_event(*controller);
            auto oth_id = other(cxt->pid, _self->id);
            auto oth_stream = model.value().get().get_helper_stream<P2PHelperStream>(_merge_id,oth_id);
            if(!oth_stream)
            {
                stop(std::format("P2P helper controller peer stream {} not found!", oth_id));
                return core::StreamVariantErrcode::failed;
            }
            oth_stream->stop_check_timeout();
            oth_stream->setup_event(*controller);
            if (controller->get().ready())
                controller->get().start();
            else {
                stop("P2P helper controller not ready!");
                return core::StreamVariantErrcode::failed;
            }
        }
        else {
            _timeout_timer = connect_cxt_->engine_cxt_->io_cxt.make_shared<io::Timer>();
            _timeout_timer->start(std::bind(&P2PHelperStream::on_timeout,this,std::placeholders::_1),1000 * 30, 0);
        }
        return core::StreamVariantErrcode::ok;
    }

    void P2PHelperStream::setup_event(mqas::tools::controller::p2p_helper_controller& controller) 
    {
        if(!*this)return;
        controller.register_event(_self->id,std::bind(&P2PHelperStream::send_connect,this, std::placeholders::_1),
            std::bind(&P2PHelperStream::send_result,this,std::placeholders::_1));
    }

    void P2PHelperStream::stop(std::optional<std::string> reason)
    {
        if (!*this || _merge_id <= 0) return;
        auto locator = locator::inst();
        auto controller = locator->get<tools::controller::p2p_helper_controller>((size_t)_merge_id);
        if(controller)
            controller->get().stop(std::move(reason));
        _self = nullptr;
        _merge_id = 0;
        _other_id = 0;
    }

    void P2PHelperStream::send_connect(const proto::p2p::NotifyConnectPeerData& msg)
    {
        send<NotifyConnectPeerDataPair>(msg);
    }

    void P2PHelperStream::send_result(const proto::p2p::NotifyConnectResult& msg)
    {
        _notified_result = true;
        send_req_quit<NotifyConnectResultPair>(stream_tag_, msg,true);
    }

    void P2PHelperStream::on_read_msg_s(const std::shared_ptr<proto::p2p::ReqSubmitRecvPeerKeyCode>& msg)
    {
        if (!*this || _merge_id <= 0) return;
        auto controller = locator::inst()->get<tools::controller::p2p_helper_controller>((size_t)_merge_id);
        if (controller && _other_id == msg->peer_id())
            controller->get().submit_verify_code(_merge_id, _self->id, msg);
    }

    core::StreamVariantErrcode P2PHelperStream::on_peer_quit(const std::span<uint8_t>& d, std::vector<uint8_t>& buf)
    {
        return core::StreamVariantErrcode::ok;
    }
}