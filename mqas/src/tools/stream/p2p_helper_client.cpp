#include "p2p_probe_path.hpp"

namespace mqas::tools::p2p {
	P2PHelperClientStream::~P2PHelperClientStream() = default;

	void P2PHelperClientStream::on_init(::lsquic_stream_t* stream, core::connect_cxt* context, std::weak_ptr<core::IConnect> connection) {
		BASE_TYPE::on_init(stream, context, connection);
		auto engine = context->engine_cxt_->engine.lock();
		if (!engine) return;
		auto socket = engine->get_socket();
		const auto self = *context->get_cxt<uint32_t>();
		auto& io = context->engine_cxt_->io_cxt;
		lobby_ = std::make_shared<LobbySocketProbePath>(io, self, socket);
		lobby_->on_probe_received.connect(sigc::mem_fun(*this, &P2PHelperClientStream::report));
		io::SocketAddress address;
		socket->get_sock_addr(address.storage);
		if (address.native().sa_family == AF_INET) {
			ipv6_ = std::make_shared<IPv6SocketProbePath>(io, self);
			ipv6_->on_probe_received.connect(sigc::mem_fun(*this, &P2PHelperClientStream::report));
		}
	}

	template<class Pair>
	core::StreamVariantErrcode P2PHelperClientStream::prepare(const std::shared_ptr<typename Pair::PB_MSG_TYPE>& msg, std::vector<uint8_t>& buf) {
		if (!msg || !lobby_) return core::StreamVariantErrcode::failed;
		peer_ = msg->peer_id();
		auto& list = *msg->mutable_ip_list();
		lobby_->prepare(list);
		if (ipv6_) ipv6_->prepare(list);
		else list.clear_additional_ipv6_endpoints();
		LOG(INFO) << "p2p_probe peer=" << peer_ << " ipv6_prepared=" << list.additional_ipv6_endpoints_size();
		return core::ProtoBufMsg::write_msg<Pair>(buf, *msg) ? core::StreamVariantErrcode::ok : core::StreamVariantErrcode::failed;
	}
	core::StreamVariantErrcode P2PHelperClientStream::on_local_change_msg_s(const std::shared_ptr<proto::p2p::ReqConnectPeer>& msg, std::vector<uint8_t>& buf) {
		return prepare<ReqConnectPeerPair>(msg, buf);
	}
	core::StreamVariantErrcode P2PHelperClientStream::on_local_change_msg_s(const std::shared_ptr<proto::p2p::ReqRespondPeerReqConnect>& msg, std::vector<uint8_t>& buf) {
		return prepare<ReqRespondPeerReqConnectPair>(msg, buf);
	}
	void P2PHelperClientStream::on_close() {
		finished_ = true;
		lobby_.reset();
		ipv6_.reset();
		BASE_TYPE::on_close();
	}
	void P2PHelperClientStream::report(const proto::p2p::ReqSubmitRecvPeerKeyCode& msg) {
		if (!finished_ && msg.peer_id() == peer_) send<ReqSubmitRecvPeerKeyCodePair>(msg);
	}
	void P2PHelperClientStream::on_read_msg_s(const std::shared_ptr<proto::p2p::NotifyP2PServerReady>& msg) { on_server_ready.emit(msg); }
	void P2PHelperClientStream::on_read_msg_s(const std::shared_ptr<proto::p2p::NotifyConnectPeerData>& msg) {
		if (finished_ || msg->peer_id() != peer_) return;
		on_connect_peer.emit(msg);
		auto& path = msg->connect_data().use_non_lobby_socket() ? ipv6_ : lobby_;
		if (lobby_) lobby_->stop();
		if (ipv6_) ipv6_->stop();
		if (path) path->start(msg);
	}
	core::StreamVariantErrcode P2PHelperClientStream::on_peer_quit_msg_s(const std::shared_ptr<proto::p2p::NotifyConnectResult>& msg, std::vector<uint8_t>&) {
		if (!finished_) {
			finished_ = true;
			auto& path = msg->use_non_lobby_socket() ? ipv6_ : lobby_;
			auto socket = path ? path->finish() : nullptr;
			lobby_.reset();
			ipv6_.reset();
			on_quit_result.emit(msg, std::move(socket));
		}
		return core::StreamVariantErrcode::ok;
	}
	void P2PHelperClientStream::on_peer_change_ack_msg_s(core::StreamVariantErrcode code, const std::shared_ptr<proto::p2p::RespondConnectPeer>& msg) {
		on_change_result.emit(msg);
		if (code != core::StreamVariantErrcode::ok) req_quit();
	}
}
