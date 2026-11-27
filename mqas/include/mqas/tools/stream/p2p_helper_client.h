#pragma once
#include "mqas/core/pb_stream.h"
#include "MsgDef.h"
#include <sigc++/sigc++.h>
#include <mqas/io/udp.h>

namespace mqas::tools::p2p {
	class P2PProbePath;
	class MQAS_EXTERN P2PHelperClientStream : public core::ProtoBufStream<P2PHelperClientStream,
		ReqConnectPeerPair, RespondConnectPeerPair,
		ReqRespondPeerReqConnectPair,
		NotifyConnectPeerDataPair, ReqSubmitRecvPeerKeyCodePair,
		NotifyConnectResultPair, NotifyP2PServerReadyPair
	> {
	public:
		sigc::signal<void(const std::shared_ptr<proto::p2p::NotifyConnectPeerData>&)> on_connect_peer;
		sigc::signal<void(const std::shared_ptr<proto::p2p::RespondConnectPeer>&)> on_change_result;
		sigc::signal<void(const std::shared_ptr<proto::p2p::NotifyConnectResult>&,std::shared_ptr<io::UdpSocket>)> on_quit_result;
		sigc::signal<void(const std::shared_ptr<proto::p2p::NotifyP2PServerReady>&)> on_server_ready;

		~P2PHelperClientStream();
		void on_close();

		void on_read_msg_s(const std::shared_ptr<proto::p2p::NotifyConnectPeerData>& msg);
		void on_read_msg_s(const std::shared_ptr<proto::p2p::NotifyP2PServerReady>& msg);
		mqas::core::StreamVariantErrcode on_peer_quit_msg_s(const std::shared_ptr<proto::p2p::NotifyConnectResult>& res,
			std::vector<uint8_t>& buf);
		void on_peer_change_ack_msg_s(core::StreamVariantErrcode code, const std::shared_ptr<proto::p2p::RespondConnectPeer>&);
		void on_init(::lsquic_stream_t* lsquic_stream, core::connect_cxt* connect_cxt, std::weak_ptr<core::IConnect> connect);
		core::StreamVariantErrcode on_local_change_msg_s(const std::shared_ptr<proto::p2p::ReqConnectPeer>&, std::vector<uint8_t>&);
		core::StreamVariantErrcode on_local_change_msg_s(const std::shared_ptr<proto::p2p::ReqRespondPeerReqConnect>&, std::vector<uint8_t>&);
	private:
		template<class Pair> core::StreamVariantErrcode prepare(const std::shared_ptr<typename Pair::PB_MSG_TYPE>&, std::vector<uint8_t>&);
		void report(const proto::p2p::ReqSubmitRecvPeerKeyCode&);
		std::shared_ptr<P2PProbePath> lobby_, ipv6_;
		uint32_t peer_ = 0;
		bool finished_ = false;
	};
}
