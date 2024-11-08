#pragma once
#include "mqas/core/pb_stream.h"
#include "MsgDef.h"
#include <memory>
#include <sigc++/sigc++.h>

namespace mqas::tools::p2p {
	class MQAS_EXTERN P2PLobbyClientStream : public core::ProtoBufStream<P2PLobbyClientStream,
		ReqRegistePeerPair, RespondRegistePeerPair,
		ReqUnregistePeerPair, RespondUnregistePeerPair,
		ReqPeerListPair, RespondPeerListPair,
		ReqConnectPeerPair,RespondConnectPeerPair,
		NotifyPeerWantConnectPair,
		ReqRespondPeerReqConnectPair
	> {


	public:
		sigc::signal<void(std::shared_ptr<proto::p2p::RespondPeerList>)> on_get_peer_list_signal;
		sigc::signal<bool(const proto::p2p::PeerData&)> on_want_connect;
		core::StreamVariantErrcode on_change_msg_s(const std::shared_ptr<proto::p2p::ReqRegistePeer>& msg,
			std::vector<uint8_t>& ret);
		std::function<void(const proto::p2p::ReqRespondPeerReqConnect&)> on_change_helper;
		std::function<void(const proto::p2p::ReqConnectPeer&)> on_change_helper_by_req;
		void on_read_msg_s(const std::shared_ptr<proto::p2p::RespondPeerList>& msg);
		void on_read_msg_s(const std::shared_ptr<proto::p2p::NotifyPeerWantConnect>& msg);
		void on_read_msg_s(const std::shared_ptr<proto::p2p::RespondConnectPeer>& msg);
		
	
		bool req_connect(uint32_t peer_id);
		void req_peer_list();
	protected:
		virtual void on_get_peer_list();
		virtual bool get_ip_list(proto::p2p::ClientIpList& res) const;
	protected:
		std::string name;
		std::shared_ptr<proto::p2p::RespondPeerList> peer_list;
	};
}
