#pragma once
#include "mqas/core/pb_stream.h"
#include "MsgDef.h"
#include <memory>
#include <sigc++/sigc++.h>
#include <unordered_set>
#include <unordered_map>

namespace mqas::tools::p2p {
	class MQAS_EXTERN P2PLobbyClientStream : public core::ProtoBufStream<P2PLobbyClientStream,
		ReqRegistePeerPair, RespondRegistePeerPair,
		ReqUnregistePeerPair, RespondUnregistePeerPair,
		ReqPeerListPair, RespondPeerListPair,
		ReqConnectPeerPair,RespondConnectPeerPair,
		NotifyPeerWantConnectPair,
		ReqRespondPeerReqConnectPair, ReqP2PServerReadyPair, NotifyP2PServerReadyPair
	> {


	public:
		sigc::signal<void(std::shared_ptr<proto::p2p::RespondPeerList>)> on_peer_list_signal;
		sigc::signal<void(std::shared_ptr<proto::p2p::RespondConnectPeer>)> on_connect_response_signal;
		sigc::signal<void(const proto::p2p::PeerData&)> on_request_connect_signal;
		sigc::signal<void(const std::shared_ptr<proto::p2p::RespondRegistePeer>&)> on_register_signal;
		sigc::signal<void(const std::shared_ptr<proto::p2p::NotifyP2PServerReady>&)> on_server_ready;

		~P2PLobbyClientStream();

		core::StreamVariantErrcode on_local_change_msg_s(const std::shared_ptr<proto::p2p::ReqRegistePeer>& msg,
			std::vector<uint8_t>& ret);
		void on_peer_change_ack_msg_s(core::StreamVariantErrcode code, const std::shared_ptr<proto::p2p::RespondRegistePeer>&);
		std::function<void(const proto::p2p::ReqRespondPeerReqConnect&)> on_change_helper;
		std::function<void(const proto::p2p::ReqConnectPeer&)> on_change_helper_by_req;
		void on_read_msg_s(const std::shared_ptr<proto::p2p::RespondPeerList>& msg);
		void on_read_msg_s(const std::shared_ptr<proto::p2p::NotifyPeerWantConnect>& msg);
		void on_read_msg_s(const std::shared_ptr<proto::p2p::RespondConnectPeer>& msg);
		void on_read_msg_s(const std::shared_ptr<proto::p2p::NotifyP2PServerReady>& msg);
		bool notify_server_ready(uint32_t peer, const std::string& negotiation);
		
	
		bool req_connect(uint32_t peer_id, proto::p2p::ServerPreference preference = proto::p2p::SERVER_NO_PREFERENCE);
		void req_peer_list();
		void req_respond(uint32_t id,bool agree, proto::p2p::ServerPreference preference = proto::p2p::SERVER_NO_PREFERENCE);
	protected:
		virtual void on_get_peer_list();
		virtual bool get_ip_list(proto::p2p::ClientIpList& res) const;
	protected:
		std::string name;
		std::shared_ptr<proto::p2p::RespondPeerList> peer_list;
		std::unordered_set<uint32_t> waiting_respond;
		std::unordered_map<uint32_t, proto::p2p::ServerPreference> pending_preferences;
		uint32_t id = 0;
	};
}
