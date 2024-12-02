#include "mqas/tools/stream/p2p_helper.h"
#include "mqas/tools/model/p2p_model.h"
#include "mqas/comm/locator.h"
#include "mqas/tools/controller/p2p_helper_controller.h"

using namespace mqas::comm;

namespace mqas::tools::p2p {


    P2PHelperStream::P2PHelperStream()
    {
        _self = connect_cxt_->get_cxt<p2p::peer_data>();
    }

    P2PHelperStream::operator bool() const
    {
        return _self != nullptr;
    }

    core::StreamVariantErrcode P2PHelperStream::on_change_msg_s(const std::shared_ptr<proto::p2p::ReqConnectPeer>& msg, std::vector<uint8_t>& ret)
    {
        return on_peer_connect(msg->peer_id(),msg->ip_list());
    }


    core::StreamVariantErrcode P2PHelperStream::on_change_msg_s(const std::shared_ptr<proto::p2p::ReqRespondPeerReqConnect>& msg,
        std::vector<uint8_t>& ret)
    {
        return on_peer_connect(msg->peer_id(), msg->ip_list());
    }

    core::StreamVariantErrcode P2PHelperStream::on_peer_connect(uint32_t id, const proto::p2p::ClientIpList & ip)
    {
        auto locator = locator::inst();
        auto model = locator->get<p2p::p2p_model>();
        if (!*this || !model)
            return core::StreamVariantErrcode::failed;

        const p2p::connect_cxt* cxt;
        _merge_id = model.value().get().reg_context(id, _self->id, ip,this->weak_from_this(), &cxt);
        if (cxt->state == p2p::ConnectState::Ready)
        {
            locator->deposit_cxt<tools::controller::p2p_helper_controller>((size_t)_merge_id, _self->id, id, connect_cxt_->engine_cxt_->io_cxt);
            auto controller = locator->get<tools::controller::p2p_helper_controller>((size_t)_merge_id);
            setup_event(*controller);
            auto oth_id = other(cxt->pid, _self->id);
            auto oth_stream = model.value().get().get_helper_stream<P2PHelperStream>(_merge_id,oth_id);
            if(!oth_stream)
            {
                stop(std::format("P2P helper controller peer stream {} not found!", oth_id));
                return core::StreamVariantErrcode::failed;
            }
            oth_stream->setup_event(*controller);
            if (controller->get().ready())
                controller->get().start();
            else {
                stop("P2P helper controller not ready!");
                return core::StreamVariantErrcode::failed;
            }
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
        auto model = locator->get<p2p::p2p_model>();
        auto controller = locator->get<tools::controller::p2p_helper_controller>((size_t)_merge_id);
        if(controller)
            controller->get().stop(std::move(reason));
        if (model)
            model->get().clear_context(_merge_id);
        _self = nullptr;
        _merge_id = 0;
    }

    void P2PHelperStream::send_connect(const proto::p2p::NotifyConnectPeerData& msg)
    {
        send<NotifyConnectPeerDataPair>(msg);
    }
    void P2PHelperStream::send_result(const proto::p2p::NotifyConnectResult& msg)
    {
        send<NotifyConnectResultPair>(msg);
    }
}