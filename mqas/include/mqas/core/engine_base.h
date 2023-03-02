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

namespace mqas::core
{
	struct MQAS_EXTERN engine_config
	{
		std::string bind_ip;
		std::string log_level;
		std::string log_path;
		std::string log_config;
		std::string alpn;
		std::string ssl_cert_path;
		std::string ssl_key_path;
		short port = 0;
		::lsquic_engine_settings lsquic_settings = {};
	};
	
	class MQAS_EXTERN IEngine
	{
	public:
			void init(void* engine_base_ptr);
			void on_new_lsquic_engine(::lsquic_engine_api&, EngineFlags);
			void on_init_socket(io::UdpSocket*);
			void on_init_logger();
			bool on_recv(const std::optional<std::span<uint8_t>>& buf, ssize_t nread, const sockaddr* addr, unsigned flags);

			void on_new_conn(void* stream_if_ctx, lsquic_conn_t* lsquic_conn);
			void on_conn_closed(lsquic_conn_t* lsquic_conn);
			void on_new_stream(void* stream_if_ctx, lsquic_stream_t* lsquic_stream);
			void on_read(lsquic_stream_t* lsquic_stream, lsquic_stream_ctx_t* lsquic_stream_ctx);
			void on_write(lsquic_stream_t* lsquic_stream, lsquic_stream_ctx_t* lsquic_stream_ctx);
			void on_close(lsquic_stream_t* lsquic_stream, lsquic_stream_ctx_t* lsquic_stream_ctx);

			//optional callback
			void on_goaway_received(lsquic_conn_t* c);
			ssize_t on_dg_write(lsquic_conn_t* c, void*, size_t);
			void on_datagram(lsquic_conn_t*, const void* buf, size_t);
			void on_hsk_done(lsquic_conn_t* c, enum lsquic_hsk_status s);
			void on_new_token(lsquic_conn_t* c, const unsigned char* token, size_t token_size);
			void on_reset(lsquic_stream_t* s, lsquic_stream_ctx_t* h, int how);
			void on_conncloseframe_received(lsquic_conn_t* c, int app_error, uint64_t error_code, const char* reason, int reason_len);
	protected:
			void* engine_base_ptr_ = nullptr;
	};

	template<typename E>
	requires requires
	{
		requires std::default_initializable<E>;
		requires std::is_base_of_v<IEngine, E>;
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
		void init(const char* conf_file, core::EngineFlags engine_flags) noexcept(false);
		void init_setting(const toml::value& conf_data);
		void init_extern_engine();
		void init_logger() const;
		int init_ssl(const char* cert_file, const char* key_file);
		void init_lsquic() noexcept(false);
		void process_conns() const;
		void start_recv() const;
		::lsquic_conn_t* connect(const ::sockaddr& addr,::lsquic_version ver, const char* hostname = nullptr, unsigned short base_plpmtu = 0,
			const unsigned char* sess_resume = nullptr, size_t sess_resume_len = 0,
			/** Resumption token: optional */
			const unsigned char* token = nullptr, size_t token_sz = 0);
		std::shared_ptr<E> get_engine() const; 
		void close_socket();
		void close_timer();
		void close_ssl_ctx();
		~engine_base()
		{
			close_socket();
			close_timer();
			close_ssl_ctx();
		}
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
		io::Context& cxt;
	protected:
		io::UdpSocket* socket_;
		io::Timer* proc_conns_timer_;
		std::shared_ptr<engine_config> conf_;
		::lsquic_engine* engine_ = nullptr;
		::SSL_CTX* ssl_ctx_ = nullptr;
		EngineFlags engine_flags_;
		::sockaddr local_addr_;
		std::shared_ptr<E> engine_extern_;
		::lsquic_logger_if lsquic_logger_if_;
		::lsquic_engine_api lsquic_engine_api_;
		::lsquic_stream_if lsquic_stream_if_;
	};
}

template<>
struct MQAS_EXTERN toml::from<mqas::core::engine_config>
{
	static mqas::core::engine_config from_toml(const value& v);
};

#include "engine_base.impl.hpp"