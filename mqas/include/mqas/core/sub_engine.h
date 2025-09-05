#pragma once
#include "engine_driver.h"
#include "engine_interface.h"
#include "mqas/tools/peer_context_mgr.h"
#include "core_concept.h"

namespace mqas::core {
	
	template<typename E,typename ED = engine_driver,typename SC = io::UdpSocket>
	requires requires
	{
		requires IsVaildEngineDriver<ED>;
		requires std::is_default_constructible_v<E>;
		requires std::is_base_of_v<IEngine, E>;
		requires IsVaildSocket<SC>;
	}
	class sub_engine : public engine_base_interface {
		friend ED; 
	public:
		~sub_engine();
		sub_engine(io::Context&);
		sub_engine(sub_engine&&) noexcept;
		sub_engine(const sub_engine&) = delete;
		sub_engine& operator=(sub_engine&&) = delete;
		sub_engine& operator=(const sub_engine&) = delete;
		void init(const char* conf_file, core::EngineFlags engine_flags, std::shared_ptr<SC> socket = nullptr) noexcept(false);
		void process_conns() const;
		void process_conns_lazy() const;
		void start_recv();
		std::shared_ptr<E> get_engine() const;
		void close();
	protected:
		void init_engine_core();
		void init_socket(std::shared_ptr<SC> sock = nullptr);
		void init_timer();
		void init_context();

		void init_config(const char* conf_file);
		bool has_engine_setting() const;


		void close_timer();
		///// lsquic event
		::lsquic_engine* get_origin() const override { return _engine;}
		EngineFlags get_engine_flags() const override { return _engine_flags;}
		void on_new_conn_s(void* stream_if_ctx, lsquic_conn_t* lsquic_conn) override;
		void on_conn_closed_s(lsquic_conn_t* lsquic_conn) override;
		void on_new_stream_s(void* stream_if_ctx, lsquic_stream_t* lsquic_stream) override;
		void on_read_s(lsquic_stream_t* lsquic_stream, lsquic_stream_ctx_t* lsquic_stream_ctx) override;
		void on_write_s(lsquic_stream_t* lsquic_stream, lsquic_stream_ctx_t* lsquic_stream_ctx) override;
		void on_close_s(lsquic_stream_t* lsquic_stream, lsquic_stream_ctx_t* lsquic_stream_ctx) override;
		//optional
		void on_goaway_received(lsquic_conn_t* c) override;
		ssize_t on_dg_write(lsquic_conn_t* c, void* buf, size_t) override;
		void on_datagram(lsquic_conn_t*, const void* buf, size_t) override;
		void on_hsk_done(lsquic_conn_t* c, enum lsquic_hsk_status s) override;
		void on_new_token(lsquic_conn_t* c, const unsigned char* token, size_t token_size) override;
		void on_reset(lsquic_stream_t* s, lsquic_stream_ctx_t* h, int how) override;
		void on_conncloseframe_received(lsquic_conn_t* c, int app_error, uint64_t error_code, const char* reason, int reason_len) override;
		/////
		static ssl_ctx_st* on_get_ssl_ctx(void* peer_ctx, const sockaddr* local);
		static int on_packets_out(void* packets_out_ctx, const lsquic_out_spec* out_spec,unsigned n_packets_out);
	public:
		std::shared_ptr<engine_cxt> context;
		io::Context& io_cxt;
	protected:
		std::shared_ptr<SC> _socket;
		io::Timer* _proc_conns_timer;
		std::shared_ptr<engine_config> _conf;
		std::shared_ptr<toml::value> _conf_origin;
		::lsquic_engine* _engine = nullptr;
		EngineFlags _engine_flags;
		::sockaddr _local_addr = {};
		std::shared_ptr<E> _engine_extern;
		::lsquic_engine_api _lsquic_engine_api = {};
		sigc::connection _recv_connection;
		::SSL_CTX* _ssl_ctx;
		tools::peer_context_mgr<sub_engine<E,ED,SC>> _peer_context_mgr;
	};

}

#include "sub_engine.impl.hpp"