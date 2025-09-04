#pragma once
#include <lsquic.h>
#include <lsquic_types.h>
#include <mqas/macro.h>
#include <mqas/io/context.h>
#include <toml.hpp>
#include <memory>
#include <mqas/core/def.h>

namespace mqas::core
{
	struct MQAS_EXTERN engine_cxt;
	class MQAS_EXTERN IEngine
	{
	public:
		void init(std::shared_ptr<engine_cxt> cxt);
		void on_new_lsquic_engine(::lsquic_engine_api&, EngineFlags);
		void on_init_socket(std::shared_ptr<io::UdpSocket> socket);
		void on_init_config(std::shared_ptr<toml::value> config);
		void on_init_engine_config(std::shared_ptr<engine_config> config);
		void on_init_logger();
		bool on_recv(const std::optional<std::span<uint8_t>>& buf, ssize_t nread, const sockaddr* addr, unsigned flags);

		void on_new_conn(void* stream_if_ctx, lsquic_conn_t* lsquic_conn);
		void on_conn_closed(lsquic_conn_t* lsquic_conn);
		void on_new_stream(void* stream_if_ctx, lsquic_stream_t* lsquic_stream);
		void on_read(lsquic_stream_t* lsquic_stream, lsquic_stream_ctx_t* lsquic_stream_ctx);
		void on_write(lsquic_stream_t* lsquic_stream, lsquic_stream_ctx_t* lsquic_stream_ctx);
		void on_close(lsquic_stream_t* lsquic_stream, lsquic_stream_ctx_t* lsquic_stream_ctx);
		void close();
		//optional callback
		void on_goaway_received(lsquic_conn_t* c);
		ssize_t on_dg_write(lsquic_conn_t* c, void*, size_t);
		void on_datagram(lsquic_conn_t*, const void* buf, size_t);
		void on_hsk_done(lsquic_conn_t* c, enum lsquic_hsk_status s);
		void on_new_token(lsquic_conn_t* c, const unsigned char* token, size_t token_size);
		void on_reset(lsquic_stream_t* s, lsquic_stream_ctx_t* h, int how);
		void on_conncloseframe_received(lsquic_conn_t* c, int app_error, uint64_t error_code, const char* reason, int reason_len);
		const std::shared_ptr<toml::value> get_config() const;
		const std::shared_ptr<io::UdpSocket> get_socket() const;
		const std::shared_ptr<engine_config> get_engine_config() const;
		std::vector<uint16_t> whitelist_port;
		std::vector<sockaddr> whitelist_addr;
	public:
		std::shared_ptr<engine_cxt> context;
	protected:
		std::shared_ptr<toml::value> config;
		std::shared_ptr<io::UdpSocket> socket_;
		std::shared_ptr<engine_config> _engine_config;
	};

}