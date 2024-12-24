#include "mqas/tools/stream/p2p_lobby_client.h"
#include "mqas/core/protobuf_msg.h"

namespace mqas::tools::p2p {


	core::StreamVariantErrcode mqas::tools::p2p::P2PLobbyClientStream::on_local_change_msg_s(const std::shared_ptr<proto::p2p::ReqRegistePeer>& msg, std::vector<uint8_t>& ret)
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

	P2PLobbyClientStream::~P2PLobbyClientStream()
	{
		connect_cxt_->set_cxt<int>(nullptr);
	}

	void P2PLobbyClientStream::on_peer_change_ret_msg_s(core::StreamVariantErrcode code, const std::shared_ptr<proto::p2p::RespondRegistePeer>& msg)
	{
		if (code == core::StreamVariantErrcode::ok && msg->ret() == proto::p2p::RetCode::ok)
		{
			id = msg->id();
			connect_cxt_->set_cxt(&id);
		}
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
		on_want_connect.emit(msg->peer());	
		waiting_respond.insert(msg->peer().id());
		return;
	}

	void P2PLobbyClientStream::on_read_msg_s(const std::shared_ptr<proto::p2p::RespondConnectPeer>& msg)
	{
		on_get_respond.emit(msg);
		if(msg->ret() == proto::p2p::RetCode::ok)
		{ 
			proto::p2p::ReqConnectPeer req_msg;
			req_msg.set_peer_id(msg->peer_id());
			proto::p2p::ClientIpList* ip_list = req_msg.mutable_ip_list();
			if (!get_ip_list(*ip_list))
			{
				LOG(ERROR) << "p2p lobby client ReqConnectPeer get ip failed";
				return;
			}
			if (on_change_helper_by_req)
				on_change_helper_by_req(req_msg);
			else
				LOG(ERROR) << "p2p lobby client on_change_helper_by_req not set";
		}
	}

	bool P2PLobbyClientStream::get_ip_list(proto::p2p::ClientIpList& res) const
	{
		const auto conn = connect.lock();
		if (!conn)
			return false;

		std::vector<std::string> list;
		mqas::io::Ip::collect_local_ip(list);

		for (auto& it : list)
		{
			auto ip = res.add_ip_list();
			ip->assign(it.c_str());
		}
		const sockaddr *local, *peer;
		conn->get_sockaddr(&local, &peer);
		res.set_port(mqas::io::Ip::addr_get_port(*local));
		return true;
	}

	void P2PLobbyClientStream::req_respond(uint32_t id, bool agree)
	{
		if (waiting_respond.find(id) == waiting_respond.end())
			return;

		proto::p2p::ReqRespondPeerReqConnect ret;
		ret.set_peer_id(id);
		auto ip_list = ret.mutable_ip_list();
		if (agree)
		{
			if(!get_ip_list(*ip_list))
				agree = false;
		}
		ret.set_agree(agree);
		if (!agree)
			send<ReqRespondPeerReqConnectPair>(ret);
		else {
			if (on_change_helper)
				on_change_helper(ret);
			else
				LOG(ERROR) << "p2p lobby client on_change_helper not set";
		}
	}

}