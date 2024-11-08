#include "mqas/tools/stream/p2p_lobby_client.h"
#include "mqas/core/protobuf_msg.h"

namespace mqas::tools::p2p {


	core::StreamVariantErrcode mqas::tools::p2p::P2PLobbyClientStream::on_change_msg_s(const std::shared_ptr<proto::p2p::ReqRegistePeer>& msg, std::vector<uint8_t>& ret)
	{
		if (name.empty())
		{
			auto engine = connect_cxt_->engine_cxt_->engine.lock();
			name = toml::find<std::string>(*engine->get_config(), "p2p", "name");
		}
		if(name.empty())
			return core::StreamVariantErrcode::failed;
		msg->set_name(name);
		core::ProtoBufMsg::write_msg<ReqRegistePeerPair>( ret,*msg );
		return core::StreamVariantErrcode::ok;
	}

	void P2PLobbyClientStream::on_read_msg_s(const std::shared_ptr<proto::p2p::RespondPeerList>& msg)
	{
		peer_list = msg;
		on_get_peer_list();
		return;
	}

	void P2PLobbyClientStream::req_peer_list()
	{
		proto::p2p::ReqPeerList msg;
		send<ReqPeerListPair>(msg);
	}

	bool P2PLobbyClientStream::req_connect(uint32_t peer_id)
	{
		proto::p2p::ReqConnectPeer msg;
		msg.set_peer_id(peer_id);
		send<ReqConnectPeerPair>(msg);
		return true;
	}

	void P2PLobbyClientStream::on_get_peer_list() 
	{
		on_get_peer_list_signal.emit(peer_list);
	}

	void P2PLobbyClientStream::on_read_msg_s(const std::shared_ptr<proto::p2p::NotifyPeerWantConnect>& msg)
	{
		proto::p2p::ReqRespondPeerReqConnect ret;

		auto want = on_want_connect.emit(msg->peer());

		ret.set_peer_id(msg->peer().id()); 
		ret.set_agree(want);
		proto::p2p::ClientIpList ip_list;
		if(want)
		{ 
			if (get_ip_list(ip_list))
				ret.set_allocated_ip_list(&ip_list);
			else
				want = false;
		}
		if(!want)
			send<ReqRespondPeerReqConnectPair>(ret);
		else {
			if (on_change_helper)
				on_change_helper(ret);
			else
				LOG(ERROR) << "p2p lobby client on_change_helper not set";
		}
		return;
	}

	void P2PLobbyClientStream::on_read_msg_s(const std::shared_ptr<proto::p2p::RespondConnectPeer>& msg)
	{
		proto::p2p::ReqConnectPeer req_msg;
		req_msg.set_peer_id(msg->peer_id());
		proto::p2p::ClientIpList ip_list;
		if (get_ip_list(ip_list))
			req_msg.set_allocated_ip_list(&ip_list);
		else
		{
			LOG(ERROR) << "p2p lobby client ReqConnectPeer get ip failed";
			return;
		}
		if (on_change_helper_by_req)
			on_change_helper_by_req(req_msg);
		else
			LOG(ERROR) << "p2p lobby client on_change_helper_by_req not set";
	}

	bool P2PLobbyClientStream::get_ip_list(proto::p2p::ClientIpList& res) const
	{
		const auto conn = connect.lock();
		if (!conn)
			return false;

		sockaddr local, peer;
		conn->get_sockaddr(local, peer);

		auto ip = res.add_ip_list();
		auto str = mqas::io::Ip::addr2str(local);
		ip->assign(str.c_str());
		
		res.set_port(mqas::io::Ip::addr_get_port(local));
		return true;
	}

}