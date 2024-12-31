#include "mqas/core/engine_interface.h"
#include "mqas/log.h"
#include "mqas/io/ip.h"
#include <algorithm>

void mqas::core::IEngine::init(std::shared_ptr<mqas::core::engine_cxt> cxt)
{
	context = std::move(cxt);
}

void mqas::core::IEngine::on_new_lsquic_engine(lsquic_engine_api&, EngineFlags) {}

void mqas::core::IEngine::on_init_socket(std::shared_ptr<io::UdpSocket> socket) {
	socket_ = std::move(socket);
}

void mqas::core::IEngine::on_init_config(std::shared_ptr<toml::value> config) {
	this->config = std::move(config);
}

void mqas::core::IEngine::on_init_logger() {}

bool mqas::core::IEngine::on_recv(const std::optional<std::span<uint8_t>>& buf, ssize_t nread, const sockaddr* addr, unsigned flags)
{
	//LOG(INFO) << "on_recv " << nread << " bytes";
	if (!whitelist_port.empty())
	{
		auto port = io::Ip::addr_get_port(*addr);
		auto it = std::find(whitelist_port.begin(),whitelist_port.end(),port);
		if(it == whitelist_port.end())
			return false;
	}
	if (!whitelist_addr.empty())
	{
		auto it = std::find_if(whitelist_addr.begin(), whitelist_addr.end(), [addr](const std::unique_ptr<sockaddr>& a){ return io::Ip::compare_ip(*addr, *a, true); });
		if (it == whitelist_addr.end())
			return false;
	}
	return true;
}

void mqas::core::IEngine::on_new_conn(void* stream_if_ctx, lsquic_conn_t* lsquic_conn)
{
#if !NDEBUG
	LOG(INFO) << "on_new_conn " << reinterpret_cast<size_t>(lsquic_conn);
#endif
}

void mqas::core::IEngine::on_conn_closed(lsquic_conn_t* lsquic_conn)
{
#if !NDEBUG
	LOG(INFO) << "on_conn_closed " << reinterpret_cast<size_t>(lsquic_conn);
#endif
}

void mqas::core::IEngine::on_new_stream(void* stream_if_ctx, lsquic_stream_t* lsquic_stream)
{
#if !NDEBUG
	LOG(INFO) << "on_new_stream " << reinterpret_cast<size_t>(lsquic_stream);
#endif
}

void mqas::core::IEngine::on_read(lsquic_stream_t* lsquic_stream, lsquic_stream_ctx_t* lsquic_stream_ctx)
{
#if !NDEBUG
	LOG(INFO) << "on_read " << reinterpret_cast<size_t>(lsquic_stream);
#endif
}

void mqas::core::IEngine::on_write(lsquic_stream_t* lsquic_stream, lsquic_stream_ctx_t* lsquic_stream_ctx)
{
#if !NDEBUG
	LOG(INFO) << "on_write " << reinterpret_cast<size_t>(lsquic_stream);
#endif
}

void mqas::core::IEngine::on_close(lsquic_stream_t* lsquic_stream, lsquic_stream_ctx_t* lsquic_stream_ctx)
{
#if !NDEBUG
	LOG(INFO) << "on_close " << reinterpret_cast<size_t>(lsquic_stream);
#endif
}
//Optional callback
void mqas::core::IEngine::on_goaway_received(lsquic_conn_t* c)
{
#if !NDEBUG
	LOG(INFO) << "on_close " << reinterpret_cast<size_t>(c);
#endif
}
ssize_t mqas::core::IEngine::on_dg_write(lsquic_conn_t* c, void* buf, size_t size)
{
#if !NDEBUG
	LOG(INFO) << "on_dg_write " << reinterpret_cast<size_t>(c) << " size = " << size;
#endif
	return 0;
}
void mqas::core::IEngine::on_datagram(lsquic_conn_t* c, const void* buf, size_t size)
{
#if !NDEBUG
	LOG(INFO) << "on_datagram " << reinterpret_cast<size_t>(c) << " size = " << size;
#endif
}
void mqas::core::IEngine::on_hsk_done(lsquic_conn_t* c, enum lsquic_hsk_status s)
{
#if !NDEBUG
	LOG(INFO) << "on_hsk_done " << reinterpret_cast<size_t>(c) << " status = " << s;
#endif
}
void mqas::core::IEngine::on_new_token(lsquic_conn_t* c, const unsigned char* token, size_t token_size)
{
#if !NDEBUG
	LOG(INFO) << "on_new_token " << reinterpret_cast<size_t>(c) << " token = " << token;
#endif
}
void mqas::core::IEngine::on_reset(lsquic_stream_t* s, lsquic_stream_ctx_t* h, int how)
{
#if !NDEBUG
	LOG(INFO) << "on_reset " << reinterpret_cast<size_t>(s) << " how = " << how;
#endif
}
void mqas::core::IEngine::on_conncloseframe_received(lsquic_conn_t* c, int app_error, uint64_t error_code, const char* reason, int reason_len)
{
#if !NDEBUG
	LOG(INFO) << "on_conncloseframe_received " << reinterpret_cast<size_t>(c) << "err_code = " << error_code << " " << reason;
#endif
}

const std::shared_ptr<toml::value> mqas::core::IEngine::get_config() const
{
	return config;
}

const std::shared_ptr<mqas::io::UdpSocket> mqas::core::IEngine::get_socket() const
{
	return socket_;
}

void mqas::core::IEngine::close() {}