#pragma once
#include "mqas/core/pb_stream.h"
#include "MsgDef.h"
#include <sigc++/sigc++.h>

namespace mqas::tools::p2p {
	class MQAS_EXTERN P2PHelperClientStream : public core::ProtoBufStream<P2PHelperClientStream,
		ReqConnectPeerPair, RespondConnectPeerPair,
		ReqRespondPeerReqConnectPair,
		NotifyConnectPeerDataPair, ReqSubmitRecvPeerKeyCodePair,
		NotifyConnectResultPair
	> {
	public:
		sigc::signal<void(const std::shared_ptr<proto::p2p::NotifyConnectPeerData>&)> on_connect_peer;
		sigc::signal<void(const std::shared_ptr<proto::p2p::RespondConnectPeer>&)> on_change_result;
		sigc::signal<void(const std::shared_ptr<proto::p2p::NotifyConnectResult>&)> on_quit_result;

		void on_read_msg_s(const std::shared_ptr<proto::p2p::NotifyConnectPeerData>& msg);
		mqas::core::StreamVariantErrcode on_peer_quit_msg_s(const std::shared_ptr<proto::p2p::NotifyConnectResult>& res,
			std::vector<uint8_t>& buf);
		void on_peer_change_ret_msg_s(core::StreamVariantErrcode code, const std::shared_ptr<proto::p2p::RespondConnectPeer>&);
	};
}