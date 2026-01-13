#include "mqas/tools/stream/p2p_helper_client.h"
#include "mqas/core/proto/simple.h"

namespace mqas::tools::p2p {
	void P2PHelperClientStream::on_read_msg_s(const std::shared_ptr<proto::p2p::NotifyConnectPeerData>& msg)
	{
		on_connect_peer.emit(msg);

		if(_timer == nullptr)
			_timer = connect_cxt_->engine_cxt_->io_cxt.make_shared<io::Timer>();

		_connect_peer_data = msg;
		_connect_count = 0;

		if (_timer->is_start())
			return;

		_timer->start([this](io::Timer* t) {
			if (!_connect_peer_data || _connect_count >= _connect_peer_data->connect_data().send_times())
				t->stop();
			send_verify_msg(_connect_peer_data);
			++_connect_count;
			if(_connect_count >= _connect_peer_data->connect_data().send_times())
				t->stop();
		},0,msg->connect_data().send_delay());
	}

	P2PHelperClientStream::~P2PHelperClientStream()
	{
		if (_receive_connect.connected())
			_receive_connect.disconnect();
	}

	mqas::core::StreamVariantErrcode P2PHelperClientStream::on_peer_quit_msg_s(const std::shared_ptr<proto::p2p::NotifyConnectResult>& res,
		std::vector<uint8_t>& buf)
	{
		if (_receive_connect.connected())
			_receive_connect.disconnect();
		on_quit_result.emit(res,_socket);
		return mqas::core::StreamVariantErrcode::ok;
	}

	void P2PHelperClientStream::on_peer_change_ack_msg_s(core::StreamVariantErrcode code, const std::shared_ptr<proto::p2p::RespondConnectPeer>& res)
	{
		on_change_result.emit(res);
		if (code != core::StreamVariantErrcode::ok)
			req_quit(stream_tag_);
	}

	void P2PHelperClientStream::on_init(::lsquic_stream_t* lsquic_stream, core::connect_cxt* connect_cxt, std::weak_ptr<core::IConnect> connect)
	{
		BASE_TYPE::on_init(lsquic_stream,connect_cxt,connect);

		auto conn = connect.lock();
		if (conn)
			conn->get_sockaddr(&_local_addr,&_remote_addr);
		auto engine = connect_cxt->engine_cxt_->engine.lock();
		if (engine)
		{
			_socket = engine->get_socket();
			_receive_connect = _socket->on_recv_signal.connect(sigc::mem_fun(*this, &P2PHelperClientStream::on_receive));
		}

		id = *connect_cxt_->get_cxt<uint32_t>();
	}

	void P2PHelperClientStream::send_verify_msg(const std::shared_ptr<proto::p2p::NotifyConnectPeerData>& msg) const
	{
		if(!_socket)
			return;
		core::proto::simple_pkg<uint32_t> pkg;

		std::array<uint8_t, sizeof(uint32_t)> code_data{};
		comm::to_big_endian(msg->connect_data().verify_code(), code_data);
		std::array<uint8_t, sizeof(uint32_t)> peer_id_data{};
		comm::to_big_endian(id, peer_id_data);
		std::array<uint8_t, sizeof(uint16_t)> ip_data{};
		comm::to_big_endian((uint16_t)msg->connect_data().ip_index(), ip_data);

		std::array<uint8_t, sizeof(uint32_t) * 2 + sizeof(uint16_t)> body{};
		std::memcpy(body.data(), code_data.data(), code_data.size());
		std::memcpy(body.data() + sizeof(uint32_t), peer_id_data.data(), peer_id_data.size());
		std::memcpy(body.data() + sizeof(uint32_t) * 2, ip_data.data(), ip_data.size());

		pkg.body = body;
		auto data = pkg.generate();
		if (data)
		{
			sockaddr addr;
			if(io::Ip::str2addr(msg->connect_data().ip().c_str(),msg->connect_data().port(),addr))
			{
				try{
					_socket->try_send(*data,addr);
				}catch(io::Exception e) {}
			}
		}
	}

	std::optional<proto::p2p::ReqSubmitRecvPeerKeyCode> P2PHelperClientStream::try_parse_verify_msg(const core::proto::simple_pkg<uint32_t>& pkg,
		const sockaddr* addr) const
	{
		if (pkg.body.size() == sizeof(uint32_t) * 2 + sizeof(uint16_t))
		{
			auto code = comm::from_big_endian<uint32_t>(std::span(&pkg.body[0], sizeof(uint32_t)));
			auto peer_id = comm::from_big_endian<uint32_t>(std::span(&pkg.body[sizeof(uint32_t)], sizeof(uint32_t)));
			auto ip_index = comm::from_big_endian<uint16_t>(std::span(&pkg.body[sizeof(uint32_t) * 2], sizeof(uint16_t)));
			

			proto::p2p::ReqSubmitRecvPeerKeyCode msg;
			msg.set_ip_index(ip_index);
			msg.set_verify_code(code);
			msg.set_peer_id(peer_id);
			auto peer_addr = msg.mutable_peer_addr();
			peer_addr->set_ip(io::Ip::addr2str(*addr));
			peer_addr->set_port(io::Ip::addr_get_port(*addr));
			#if !NDEBUG
			LOG(INFO) << "ReqSubmitRecvPeerKeyCode " << "peer addr = " << peer_addr->ip() << ':' << peer_addr->port() << " ip idx = " << ip_index;
			#endif
			return { msg };
		}

		return {};
	}

	void P2PHelperClientStream::on_receive(io::UdpSocket* sock, const std::optional<std::span<uint8_t>>& buf, ssize_t nread, const sockaddr* addr, unsigned flags)
	{
		if (nread == 0 || addr == nullptr || nread < 0)
			return;
		if (!is_stream_addr(addr) && buf)
		{
			auto [pkg,size] = core::proto::simple_pkg<uint32_t>::parse_command(*buf);
			if (pkg)
			{
				auto msg = try_parse_verify_msg(*pkg, addr);
				if (msg)
					send<ReqSubmitRecvPeerKeyCodePair>(*msg);
			}
		}
	}

	bool P2PHelperClientStream::is_stream_addr(const sockaddr* addr) const
	{
		return io::Ip::compare_ip(*_local_addr,*addr);
	}


}