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

		const auto id = model.value().get().registe_client(msg->name(),io::Ip::addr2str(peer),io::Ip::addr_get_port(peer));
		proto::p2p::RespondRegistePeer ret_msg;
		if (id == 0)
		{ 
			ret_msg.set_ret(proto::p2p::peer_rejected);
			return core::StreamVariantErrcode::failed;
		}
		if(!core::ProtoBufMsg::write_msg<RespondRegistePeerPair>(ret, ret_msg))
			return core::StreamVariantErrcode::parse_failed;
		comm::locator::inst()->deposit_cxt<uint32_t>(*this,id);
		return core::StreamVariantErrcode::ok;
	}

	core::StreamVariantErrcode P2PLobbyStream::on_peer_quit_msg_s(const std::shared_ptr<proto::p2p::ReqUnregistePeer>& msg,
		std::vector<uint8_t>& ret)
	{
		auto model = comm::locator::inst()->get<p2p_model>();
		auto id = comm::locator::inst()->get<uint32_t>(*this);
		if (!model || !id)
			return core::StreamVariantErrcode::failed;

		const auto res = model.value().get().unregiste_client(id.value());
		proto::p2p::RespondUnregistePeer ret_msg;
		if (!res)
		{
			ret_msg.set_ret(proto::p2p::peer_rejected);
			return core::StreamVariantErrcode::failed;
		}
		comm::locator::inst()->clear_by_context(*this);
		if (!core::ProtoBufMsg::write_msg<RespondUnregistePeerPair>(ret, ret_msg))
			return core::StreamVariantErrcode::parse_failed;
		return core::StreamVariantErrcode::ok;
	}

	core::StreamVariantErrcode P2PLobbyStream::on_read_msg_s(const std::shared_ptr<proto::p2p::ReqPeerList>& msg)
	{
		auto model = comm::locator::inst()->get<p2p_model>();
		auto id = comm::locator::inst()->get<uint32_t>(*this);
		if (!model || !id)
			return core::StreamVariantErrcode::failed;
		proto::p2p::RespondPeerList ret_msg;

		model.value().get().visit_client([&ret_msg,id](const p2p::peer_data& d)
		{
			if(id && *id == d.id)
				return;
			auto peer = ret_msg.add_peer_list();
			peer->set_id(d.id);
			peer->set_name(d.name);
		});

		send<RespondPeerListPair>(ret_msg);

		return core::StreamVariantErrcode::ok;
	}

}