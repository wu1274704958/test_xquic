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
		static_assert(sizeof(size_t) == sizeof(int*) && sizeof(int*) == sizeof(uint64_t),"not support!!!");
		//second
		core::StreamVariantErrcode on_change_msg_s(const std::shared_ptr<proto::p2p::ReqConnectPeer>& msg,
			std::vector<uint8_t>& ret);
		//first 
		core::StreamVariantErrcode on_change_msg_s(const std::shared_ptr<proto::p2p::ReqRespondPeerReqConnect>& msg,
			std::vector<uint8_t>& ret);
	protected:
		core::StreamVariantErrcode on_peer_connect(uint32_t id,const proto::p2p::ClientIpList& ip);
	protected:

	};
}