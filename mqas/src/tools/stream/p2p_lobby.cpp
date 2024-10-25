#include "mqas/tools/stream/p2p_lobby.h"

namespace mqas::tools::p2p {

	core::StreamVariantErrcode P2PLobbyStream::on_change_msg_s(const std::shared_ptr<proto::p2p::ReqRegistePeer>& msg,
		std::vector<uint8_t>& ret)
	{

	}

	core::StreamVariantErrcode P2PLobbyStream::on_peer_quit_msg_s(const std::shared_ptr<proto::p2p::ReqUnregistePeer>& msg,
		std::vector<uint8_t>& ret)
	{

	}

	core::StreamVariantErrcode P2PLobbyStream::on_read_msg_s(const std::shared_ptr<proto::p2p::ReqPeerList>& msg)
	{

	}

	core::StreamVariantErrcode P2PLobbyStream::on_read_msg_s(const std::shared_ptr<proto::p2p::ReqConnectPeer>& msg)
	{

	}

}