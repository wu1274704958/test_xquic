#include "mqas/tools/stream/p2p_helper_client.h"


namespace mqas::tools::p2p {
	void P2PHelperClientStream::on_read_msg_s(const std::shared_ptr<proto::p2p::NotifyConnectPeerData>& msg)
	{
		on_connect_peer.emit(msg);
	}

	void P2PHelperClientStream::on_peer_quit_ret_msg_s(core::StreamVariantErrcode e, const std::shared_ptr<proto::p2p::NotifyConnectResult>& res)
	{
		on_quit_result.emit(res);
	}

	void P2PHelperClientStream::on_peer_change_ret_msg_s(core::StreamVariantErrcode code, const std::shared_ptr<proto::p2p::RespondConnectPeer>& res)
	{
		on_change_result.emit(res);
		if (code != core::StreamVariantErrcode::ok)
			req_quit(stream_tag_);
	}
}