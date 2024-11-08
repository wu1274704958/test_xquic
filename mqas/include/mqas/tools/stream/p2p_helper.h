#pragma once
#include "mqas/core/pb_stream.h"
#include "MsgDef.h"
#include <memory>

namespace mqas::tools::p2p {
	class p2p_model;

	class MQAS_EXTERN P2PHelperStream : public core::ProtoBufStream<
		ReqConnectPeerPair, RespondConnectPeerPair,
		NotifyPeerWantConnectPair, ReqRespondPeerReqConnectPair,
		NotifyConnectPeerDataPair, ReqSubmitRecvPeerKeyCodePair,
		NotifyConnectResultPair
	> {
		core::StreamVariantErrcode on_change_msg_s(const std::shared_ptr<proto::p2p::ReqConnectPeer>& msg,
			std::vector<uint8_t>& ret);
	};
}