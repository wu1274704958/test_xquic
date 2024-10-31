#pragma once
#include "mqas/core/pb_stream.h"
#include "MsgDef.h"
#include <memory>



namespace mqas::tools::p2p {

	class p2p_model;

	class MQAS_EXTERN P2PLobbyStream : public core::ProtoBufStream<P2PLobbyStream,
		ReqRegistePeerPair,RespondRegistePeerPair,
		ReqUnregistePeerPair,RespondUnregistePeerPair,
		ReqPeerListPair,RespondPeerListPair
	> {
	public:
		core::StreamVariantErrcode on_change_msg_s(const std::shared_ptr<proto::p2p::ReqRegistePeer>& msg,
			std::vector<uint8_t>& ret);

		core::StreamVariantErrcode on_peer_quit_msg_s(const std::shared_ptr<proto::p2p::ReqUnregistePeer>& msg,
			std::vector<uint8_t>& ret);

		core::StreamVariantErrcode on_read_msg_s(const std::shared_ptr<proto::p2p::ReqPeerList>& msg);
		void on_close();
	protected:
		uint32_t id = 0; 
		std::shared_ptr<p2p_model> model;
	};
}