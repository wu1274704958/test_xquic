#pragma once
#include "mqas/core/pb_stream.h"
#include "MsgDef.h"
#include <memory>
#include "mqas/tools/controller/p2p_helper_controller.h"

namespace mqas::tools::p2p {
	class p2p_model;
	class peer_data;

	class MQAS_EXTERN P2PHelperStream : public core::ProtoBufStream<
		ReqConnectPeerPair, RespondConnectPeerPair,
		NotifyPeerWantConnectPair, ReqRespondPeerReqConnectPair,
		NotifyConnectPeerDataPair, ReqSubmitRecvPeerKeyCodePair,
		NotifyConnectResultPair
	> {

		static_assert(sizeof(size_t) == sizeof(int*) && sizeof(int*) == sizeof(uint64_t),"not support!!!");
		public:
		P2PHelperStream();
		operator bool() const;

		//second
		core::StreamVariantErrcode on_change_msg_s(const std::shared_ptr<proto::p2p::ReqConnectPeer>& msg,
			std::vector<uint8_t>& ret);
		//first 
		core::StreamVariantErrcode on_change_msg_s(const std::shared_ptr<proto::p2p::ReqRespondPeerReqConnect>& msg,
			std::vector<uint8_t>& ret);
	protected:
		core::StreamVariantErrcode on_peer_connect(uint32_t id,const proto::p2p::ClientIpList& ip);
		void setup_event(mqas::tools::controller::p2p_helper_controller& controller) ;
		void stop(std::optional<std::string> reason);
		void send_connect(const proto::p2p::NotifyConnectPeerData& msg);
		void send_result(const proto::p2p::NotifyConnectResult& msg);
	protected:
		p2p::peer_data* _self = nullptr;
		uint64_t _merge_id = 0;
	};
}