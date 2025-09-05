#pragma once
#include <lsquic.h>
#include <lsquic_types.h>
#include <mqas/macro.h>
#include <mqas/io/context.h>
#include <toml.hpp>
#include <memory>
#include <mqas/core/def.h>
#include <openssl/ssl.h>
#include <concepts>
#include <span>
#include <optional>
#include <sigc++/sigc++.h>
#include "engine_interface.h"
#include "engine_driver.h"
#include "mqas/tools/peer_context_mgr.h"
#include "core_concept.h"

namespace mqas::core
{
	template<typename E, typename ED = engine_driver,typename SC = io::UdpSocket>
	requires requires
	{
		requires mqas::core::IsVaildEngineDriver<ED>;
		requires std::is_default_constructible_v<E>;
		requires std::is_base_of_v<mqas::core::IEngine, E>;
		requires mqas::core::IsVaildSocket<SC>;
	}
	class engine_base
	{
		friend E;
	public:
		engine_base(io::Context&);
		engine_base(engine_base&&) noexcept;
		engine_base(const engine_base&) = delete;
		engine_base& operator=(engine_base&&) = delete;
		engine_base& operator=(const engine_base&) = delete;
		void init(const char* conf_file, core::EngineFlags engine_flags,
			std::shared_ptr<io::UdpSocket> socket = nullptr) noexcept(false);
		void process_conns() const;
        void process_conns_lazy() const;
		void start_recv();
		std::shared_ptr<E> get_engine() const;
		void close();
		~engine_base();
	protected:
		void init_socket(std::shared_ptr<io::UdpSocket> sock = nullptr);
		void init_config(const char* conf_file);
		bool has_engine_setting() const;
		void init_timer();
		void init_engine_core();

		void init_logger() const;

		void init_lsquic() noexcept(false);
		void init_context();

		void close_socket();
		void close_timer();
		void close_ssl_ctx();
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
		//optional callback
		static void on_goaway_received(lsquic_conn_t* c);
		static ssize_t on_dg_write(lsquic_conn_t* c, void* buf, size_t);
		static void on_datagram(lsquic_conn_t*, const void* buf, size_t);
		static void on_hsk_done(lsquic_conn_t* c, enum lsquic_hsk_status s);
		static void on_new_token(lsquic_conn_t* c, const unsigned char* token,size_t token_size);
		static void on_reset(lsquic_stream_t* s, lsquic_stream_ctx_t* h, int how);
		static void on_conncloseframe_received(lsquic_conn_t* c,int app_error, uint64_t error_code,const char* reason, int reason_len);

		static int on_packets_out(void* packets_out_ctx, const lsquic_out_spec* out_spec, unsigned n_packets_out);
		static ssl_ctx_st* on_get_ssl_ctx(void* peer_ctx, const sockaddr* local);
	public:
		io::Context& io_cxt;
		std::shared_ptr<engine_cxt> context;
	protected:
		std::shared_ptr<SC> socket_;
		io::Timer* proc_conns_timer_;
		std::shared_ptr<engine_config> conf_;
		std::shared_ptr<toml::value> conf_origin_;
		::lsquic_engine* engine_ = nullptr;
		::SSL_CTX* ssl_ctx_ = nullptr;
		EngineFlags engine_flags_;
		::sockaddr local_addr_;
		std::shared_ptr<E> engine_extern_;
		::lsquic_logger_if lsquic_logger_if_;
		::lsquic_engine_api lsquic_engine_api_;
		::lsquic_stream_if lsquic_stream_if_;
		sigc::connection recv_connection_;
		tools::peer_context_mgr<engine_base<E,ED>> _peer_context_mgr;
	};
}

#include "engine_base.impl.hpp"