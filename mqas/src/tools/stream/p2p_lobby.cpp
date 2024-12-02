#include "mqas/tools/stream/p2p_lobby.h"
#include "mqas/tools/model/p2p_model.h"
#include "mqas/comm/locator.h"
#include "mqas/io/ip.h"

namespace mqas::tools::p2p {

	core::StreamVariantErrcode P2PLobbyStream::on_change_msg_s(const std::shared_ptr<proto::p2p::ReqRegistePeer>& msg,
		std::vector<uint8_t>& ret)
	{
		auto model = comm::locator::inst()->get<p2p_model>();
		const auto conn = connect.lock();
		if (!model || !conn || msg->name().empty())
			return core::StreamVariantErrcode::failed;

		sockaddr local,peer;
		conn->get_sockaddr(local,peer);

		id = model.value().get().registe_client(msg->name(),io::Ip::addr2str(peer),io::Ip::addr_get_port(peer),this->weak_from_this());
		proto::p2p::RespondRegistePeer ret_msg;
		if (id == 0)
		{ 
			ret_msg.set_ret(proto::p2p::peer_rejected);
			return core::StreamVariantErrcode::failed;
		}
		if(!core::ProtoBufMsg::write_msg<RespondRegistePeerPair>(ret, ret_msg))
			return core::StreamVariantErrcode::parse_failed;

		connect_cxt_->set_cxt(model.value().get()[id]);
		return core::StreamVariantErrcode::ok;
	}

	core::StreamVariantErrcode P2PLobbyStream::on_peer_quit_msg_s(const std::shared_ptr<proto::p2p::ReqUnregistePeer>& msg,
		std::vector<uint8_t>& ret)
	{
		auto model = comm::locator::inst()->get<p2p_model>();
		if (!model)
			return core::StreamVariantErrcode::failed;

		const auto res = model.value().get().unregiste_client(id);
		proto::p2p::RespondUnregistePeer ret_msg;
		if (!res)
		{
			ret_msg.set_ret(proto::p2p::peer_rejected);
			return core::StreamVariantErrcode::failed;
		}
		id = 0;
		if (!core::ProtoBufMsg::write_msg<RespondUnregistePeerPair>(ret, ret_msg))
			return core::StreamVariantErrcode::parse_failed;
		return core::StreamVariantErrcode::ok;
	}

	void P2PLobbyStream::on_read_msg_s(const std::shared_ptr<proto::p2p::ReqPeerList>& msg)
	{
		auto model = comm::locator::inst()->get<p2p_model>();
		if (!model)
			return;
		proto::p2p::RespondPeerList ret_msg;

		model.value().get().visit_client([&ret_msg,this](const p2p::peer_data& d)
		{
			if(id == d.id)
				return;
			auto peer = ret_msg.add_peer_list();
			peer->set_id(d.id);
			peer->set_name(d.name);
		});

		send<RespondPeerListPair>(ret_msg);
	}

	void P2PLobbyStream::on_close()
	{
		auto model = comm::locator::inst()->get<p2p_model>();
		if (id > 0 && model)
		{
			model.value().get().unregiste_client(id);
		}
		IStream::on_close();
	}

	void P2PLobbyStream::on_read_msg_s(const std::shared_ptr<proto::p2p::ReqConnectPeer>& msg)
	{
		auto model = comm::locator::inst()->get<p2p_model>();
		if (!model)return;
		auto self = model.value().get()[id];
		auto res = model.value().get().visit_client_stream<P2PLobbyStream>(msg->peer_id(), [&self,this](std::shared_ptr<P2PLobbyStream> ptr)->bool {
			proto::p2p::NotifyPeerWantConnect m2;
			auto peer = m2.mutable_peer();
			peer->set_id(id);
			peer->set_name(self->name);
			ptr->send_lazy<NotifyPeerWantConnectPair>(m2);
			return true;
		});
		if (!res)
		{
			proto::p2p::RespondConnectPeer ret;
			ret.set_ret(proto::p2p::not_exists);
			send<RespondConnectPeerPair>(ret);
		}
	}

	void P2PLobbyStream::on_read_msg_s(const std::shared_ptr<proto::p2p::ReqRespondPeerReqConnect>& msg)
	{
		auto model = comm::locator::inst()->get<p2p_model>();
		if (!model)return;
		if (msg->agree())
		{
			LOG(ERROR) << "p2p lobby ReqRespondPeerReqConnect request agree must change to p2p helper!!!";
		}
		else {
			model.value().get().visit_client_stream<P2PLobbyStream>(msg->peer_id(), [msg, this](std::shared_ptr<P2PLobbyStream> ptr)->bool {
				ptr->send_respond_for_req_connect(id, proto::p2p::RetCode::peer_rejected);
				return true;
			});
		}
	}

	bool P2PLobbyStream::send_respond_for_req_connect(uint32_t id, proto::p2p::RetCode code)
	{
		proto::p2p::RespondConnectPeer ret;
		ret.set_peer_id(id);
		ret.set_ret(code);
		return send_lazy<RespondConnectPeerPair>(ret);
	}

}