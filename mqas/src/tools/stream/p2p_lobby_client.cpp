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

	core::StreamVariantErrcode P2PLobbyClientStream::on_read_msg_s(const std::shared_ptr<proto::p2p::RespondPeerList>& msg)
	{
		peer_list = msg;
		on_get_peer_list();
		return core::StreamVariantErrcode::ok;
	}

	void P2PLobbyClientStream::req_peer_list()
	{
		proto::p2p::ReqPeerList msg;
		send<ReqPeerListPair>(msg);
	}

	bool P2PLobbyClientStream::req_connect(uint32_t peer_id)
	{
		return true;
	}

	void P2PLobbyClientStream::on_get_peer_list() {

	}

}