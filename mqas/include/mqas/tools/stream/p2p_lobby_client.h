#pragma once
#include "mqas/core/pb_stream.h"
#include "MsgDef.h"
#include <memory>

namespace mqas::tools::p2p {
	class MQAS_EXTERN P2PLobbyClientStream : public core::ProtoBufStream<P2PLobbyClientStream,
		ReqRegistePeerPair, RespondRegistePeerPair,
		ReqUnregistePeerPair, RespondUnregistePeerPair,
		ReqPeerListPair, RespondPeerListPair
	> {


	public:
		core::StreamVariantErrcode on_change_msg_s(const std::shared_ptr<proto::p2p::ReqRegistePeer>& msg,
			std::vector<uint8_t>& ret);

		core::StreamVariantErrcode on_read_msg_s(const std::shared_ptr<proto::p2p::RespondPeerList>& msg);
	
		bool req_connect(uint32_t peer_id);
		void req_peer_list();
		void print_peer_list() const;
		P2PLobbyClientStream();
	protected:
		std::string name;
		std::shared_ptr<proto::p2p::RespondPeerList> peer_list;
	};
}