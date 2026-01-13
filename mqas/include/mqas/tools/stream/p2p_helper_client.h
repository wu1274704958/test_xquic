#pragma once
#include "mqas/core/pb_stream.h"
#include "MsgDef.h"
#include <sigc++/sigc++.h>
#include <mqas/io/udp.h>

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
		sigc::signal<void(const std::shared_ptr<proto::p2p::NotifyConnectResult>&,std::shared_ptr<io::UdpSocket>)> on_quit_result;

		~P2PHelperClientStream();

		void on_read_msg_s(const std::shared_ptr<proto::p2p::NotifyConnectPeerData>& msg);
		mqas::core::StreamVariantErrcode on_peer_quit_msg_s(const std::shared_ptr<proto::p2p::NotifyConnectResult>& res,
			std::vector<uint8_t>& buf);
		void on_peer_change_ack_msg_s(core::StreamVariantErrcode code, const std::shared_ptr<proto::p2p::RespondConnectPeer>&);
		void on_init(::lsquic_stream_t* lsquic_stream, core::connect_cxt* connect_cxt, std::weak_ptr<core::IConnect> connect);
	protected:
		void send_verify_msg(const std::shared_ptr<proto::p2p::NotifyConnectPeerData>&) const;
		std::optional<proto::p2p::ReqSubmitRecvPeerKeyCode> try_parse_verify_msg(const core::proto::simple_pkg<uint32_t>& pkg, const sockaddr* addr) const;
		void on_receive(io::UdpSocket* sock, const std::optional<std::span<uint8_t>>& buf, ssize_t nread, const sockaddr* addr, unsigned flags);
		bool is_stream_addr(const sockaddr* addr) const;

	protected:
		const sockaddr* _local_addr; 
		const sockaddr* _remote_addr;
		std::shared_ptr<io::UdpSocket> _socket;
		sigc::connection _receive_connect;
		std::shared_ptr<proto::p2p::NotifyConnectPeerData> _connect_peer_data;
		std::shared_ptr<io::Timer> _timer;
		uint32_t _connect_count = 0;
		uint32_t id;
	};
}