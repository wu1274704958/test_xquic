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
	struct MQAS_EXTERN engine_config
	{
		std::string bind_ip;
		short port;
		std::string log_level;
		std::string log_path;
		std::string log_config;
		std::string alpn;
	};

	class MQAS_EXTERN engine_base
	{
	public:
		engine_base(io::Context&);
		engine_base(engine_base&&) noexcept;
		engine_base(const engine_base&) = delete;
		engine_base& operator=(engine_base&&) = delete;
		engine_base& operator=(const engine_base&) = delete;
		void init(const char* conf_file, core::EngineFlags engine_flags) noexcept(false);
		void init_lsquic(core::EngineFlags engine_flags) noexcept(false);
		void close_socket();
		void close_timer();
		~engine_base();
	protected:
		static std::string load_config(const char* conf_file);
		//lsquic callback function
		static int lsquic_log_func(void* logger_ctx, const char* buf, size_t len);

		static lsquic_conn_ctx_t* on_new_conn_s(void* stream_if_ctx, lsquic_conn_t* lsquic_conn);
		static void on_conn_closed_s(lsquic_conn_t* lsquic_conn);
		static lsquic_stream_ctx_t* on_new_stream_s(void* stream_if_ctx, lsquic_stream_t* lsquic_stream);
		static void on_read_s(lsquic_stream_t* lsquic_stream, lsquic_stream_ctx_t* lsquic_stream_ctx);
		static void on_write_s(lsquic_stream_t* lsquic_stream, lsquic_stream_ctx_t* lsquic_stream_ctx);
		static void on_close_s(lsquic_stream_t* lsquic_stream, lsquic_stream_ctx_t* lsquic_stream_ctx);
		static int on_packets_out(void* packets_out_ctx, const lsquic_out_spec* out_spec, unsigned n_packets_out);
		static ssl_ctx_st* on_get_ssl_ctx(void* peer_ctx, const sockaddr* local);
	public:
		io::Context& cxt;
	protected:
		io::UdpSocket* socket_;
		io::Timer* proc_conns_timer_;
		std::shared_ptr<engine_config> conf_;
		::lsquic_engine* engine_ = nullptr;
	};

}

template<>
struct toml::from<mqas::core::engine_config>
{
	static mqas::core::engine_config from_toml(const value& v);
}
;
