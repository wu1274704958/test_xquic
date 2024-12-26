#pragma once
#include "..\macro.h"
#include <toml.hpp>
#include <lsquic.h>
#include <openssl/ssl.h>
#include "def.h"
#include <mutex>
#include <atomic>
#include <unordered_map>

namespace mqas::core
{
	struct MQAS_EXTERN engine_config;

	class MQAS_EXTERN engine_base_interface
	{
		public:
			virtual ::lsquic_engine* get_origin() const = 0;
			virtual EngineFlags get_engine_flags() const = 0;
			virtual void on_new_conn_s(void* stream_if_ctx, lsquic_conn_t* lsquic_conn) = 0;
			virtual void on_conn_closed_s(lsquic_conn_t* lsquic_conn) = 0;
			virtual void on_new_stream_s(void* stream_if_ctx, lsquic_stream_t* lsquic_stream) = 0;
			virtual void on_read_s(lsquic_stream_t* lsquic_stream, lsquic_stream_ctx_t* lsquic_stream_ctx) = 0;
			virtual void on_write_s(lsquic_stream_t* lsquic_stream, lsquic_stream_ctx_t* lsquic_stream_ctx) = 0;
			virtual void on_close_s(lsquic_stream_t* lsquic_stream, lsquic_stream_ctx_t* lsquic_stream_ctx) = 0;
			//optional callback
			virtual void on_goaway_received(lsquic_conn_t* c) = 0;
			virtual ssize_t on_dg_write(lsquic_conn_t* c, void* buf, size_t) = 0;
			virtual void on_datagram(lsquic_conn_t*, const void* buf, size_t) = 0;
			virtual void on_hsk_done(lsquic_conn_t* c, enum lsquic_hsk_status s) = 0;
			virtual void on_new_token(lsquic_conn_t* c, const unsigned char* token, size_t token_size) = 0;
			virtual void on_reset(lsquic_stream_t* s, lsquic_stream_ctx_t* h, int how) = 0;
			virtual void on_conncloseframe_received(lsquic_conn_t* c, int app_error, uint64_t error_code, const char* reason, int reason_len) = 0;
	};

	class MQAS_EXTERN base_engine_driver {
	public:
		bool initialized() const { return false; }
		const toml::value* get_global_conf() const { return nullptr; }
		bool register_engine(engine_base_interface* e, ::lsquic_engine* origin_e, const char* conf) {return false;}
		void unregister_engine(engine_base_interface* e, ::lsquic_engine* origin_e) {}
		void fill_lsquic_engine_api(::lsquic_engine* origin_e,EngineFlags flags,::lsquic_engine_api& api) const {}
	};

	template <typename T>
	concept IsVaildEngineDriver = requires() {
		requires std::is_base_of_v<base_engine_driver,T>;
		T::instance();
		requires std::is_same_v<std::shared_ptr<T>,std::remove_cv_t<decltype(T::instance())>>;
	};

	class MQAS_EXTERN engine_driver : public base_engine_driver
	{
		public:
		static std::shared_ptr<engine_driver> instance();
		bool initialized() {return _initialized;}
		const toml::value* get_global_conf() const { return &_golbal_conf;}
		bool register_engine(engine_base_interface* e, ::lsquic_engine* origin_e,const char* conf);
		void unregister_engine(engine_base_interface* e, ::lsquic_engine* origin_e);
		void fill_lsquic_engine_api(::lsquic_engine* origin_e,EngineFlags flags, ::lsquic_engine_api& api) const;

		protected:
		bool initialization(const char* conf, EngineFlags flags);
		void init_setting(const toml::value& conf_data, EngineFlags flags);
		void init_logger() const;
		int init_ssl(const char* cert_file, const char* key_file);
		void init_lsquic() noexcept(false);

		static void settings_from_toml(::lsquic_engine_settings& s, const toml::value& v);
		static int ssl_select_alpn_s(::SSL* ssl, const unsigned char** out, unsigned char* outlen,
			const unsigned char* in, unsigned inlen, void* arg);
		static engine_base_interface* get_engine_by_cxt(void* cxt, ::lsquic_engine* engine);

		protected:
			static std::shared_ptr<engine_driver> _instance;
			static std::mutex _instance_lock;

			engine_config _engine_config;
			toml::value _golbal_conf;
			std::atomic_bool _initialized = false;
			//context
			::SSL_CTX* _ssl_ctx = nullptr;
			::lsquic_logger_if _lsquic_logger_if;
			::lsquic_stream_if _lsquic_stream_if;
			
			std::unordered_map<::lsquic_engine*,engine_base_interface*> _engine_map;

		protected:
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
			static void on_new_token(lsquic_conn_t* c, const unsigned char* token, size_t token_size);
			static void on_reset(lsquic_stream_t* s, lsquic_stream_ctx_t* h, int how);
			static void on_conncloseframe_received(lsquic_conn_t* c, int app_error, uint64_t error_code, const char* reason, int reason_len);

			static int on_packets_out(void* packets_out_ctx, const lsquic_out_spec* out_spec, unsigned n_packets_out);
			static ssl_ctx_st* on_get_ssl_ctx(void* peer_ctx, const sockaddr* local);
	};

	
}



template<>
struct MQAS_EXTERN toml::from<mqas::core::engine_config>
{
	static mqas::core::engine_config from_toml(const value& v);
};