#pragma once

#include <fstream>
#include <sstream>
#include <stdexcept>
#include <mqas/io/udp.h>
#include <mqas/io/timer.h>
#include "easylogging++.h"
#include <lsquic.h>
#include <mqas/log.h>
#include <mqas/comm/macro.h>
#include <mqas/io/ip.h>

#ifdef PF_ANDROID
#endif

void MQAS_EXTERN settings_from_toml(::lsquic_engine_settings& s, const toml::value& v);
int MQAS_EXTERN ssl_select_alpn_s(SSL* ssl, const unsigned char** out, unsigned char* outlen,
	const unsigned char* in, unsigned inlen, void* arg);

#define ENGINE_BASE_TEMPLATE_DECL											\
template<typename E>														\
requires requires															\
{																			\
	E();																	\
	requires std::is_base_of_v<mqas::core::IEngine, E>;						\
}

ENGINE_BASE_TEMPLATE_DECL
mqas::core::engine_base<E>::engine_base(io::Context& c):cxt(c),socket_(nullptr)
,proc_conns_timer_(nullptr)
{}

ENGINE_BASE_TEMPLATE_DECL
mqas::core::engine_base<E>::engine_base(engine_base&& oth) noexcept : cxt(oth.cxt)
{
	socket_ = oth.socket_;
	oth.socket_ = nullptr;

	proc_conns_timer_ = oth.proc_conns_timer_;
	oth.proc_conns_timer_ = nullptr;
}
ENGINE_BASE_TEMPLATE_DECL
void mqas::core::engine_base<E>::init(const char* conf_file,core::EngineFlags engine_flags)
{
	//parse config 
	const auto conf_data = toml::parse(conf_file);
	conf_ = std::make_shared<engine_config>(toml::find<engine_config>(conf_data, "engine_config"));
	
	::lsquic_engine_init_settings(&conf_->lsquic_settings, static_cast<unsigned>(engine_flags));
	if(conf_data.contains("lsquic_settings"))
		settings_from_toml(conf_->lsquic_settings,conf_data.at("lsquic_settings"));
	//init socket
	socket_ = cxt.make_handle<io::UdpSocket>();
	sockaddr addr{};
	io::Ip::str2addr_ipv4(conf_->bind_ip.c_str(),conf_->port, addr);
	socket_->bind(addr,UV_UDP_REUSEADDR);
	// init timer
	proc_conns_timer_ = cxt.make_handle<io::Timer>();
	//init logger
	init_logger();
	//init ssl
	if (contain<uint32_t>(engine_flags, EngineFlags::Server) && !conf_->ssl_cert_path.empty() && !conf_->ssl_key_path.empty())
		init_ssl(conf_->ssl_cert_path.c_str(),conf_->ssl_key_path.c_str());
	init_lsquic(engine_flags);
}
ENGINE_BASE_TEMPLATE_DECL
void mqas::core::engine_base<E>::init_logger() const
{
	mqas::log::init("default",conf_->log_config,std::nullopt);
	const auto lsquic_log = el::Loggers::getLogger("lsquic");
	el::Configurations c;
	c.setFromBase(el::Loggers::getLogger("default")->configurations());
	auto fmt = c.get(el::Level::Global,el::ConfigurationType::Format)->value();
	auto p = fmt.find("[%level]");
	if(p != std::string::npos){
		fmt.replace(p, sizeof("[%level]"), "");
		c.set(el::Level::Global,el::ConfigurationType::Format,fmt);
	}
	el::Loggers::reconfigureLogger(lsquic_log, c);
}
ENGINE_BASE_TEMPLATE_DECL
void mqas::core::engine_base<E>::init_lsquic(core::EngineFlags flags) noexcept(false)
{
	const ::lsquic_logger_if logger_if = { lsquic_log_func, };

	lsquic_logger_init(&logger_if, this, LLTS_NONE);
	lsquic_set_log_level(conf_->log_level.c_str());

	::lsquic_stream_if stream_if = {};
	stream_if.on_new_conn = on_new_conn_s;
	stream_if.on_conn_closed = on_conn_closed_s;
	stream_if.on_new_stream = on_new_stream_s;
	stream_if.on_read = on_read_s;
	stream_if.on_write = on_write_s;
	stream_if.on_close = on_close_s;

	::lsquic_engine_api engine_api = {};
	engine_api.ea_settings = &conf_->lsquic_settings;
	engine_api.ea_packets_out = on_packets_out;
	engine_api.ea_packets_out_ctx = this;
	engine_api.ea_stream_if = &stream_if;
	engine_api.ea_stream_if_ctx = this;
	if(contain<uint32_t>(flags,EngineFlags::Server))
		engine_api.ea_get_ssl_ctx = on_get_ssl_ctx;
	if(flags == EngineFlags::None)
	{
		engine_api.ea_alpn = conf_->alpn.c_str();
		if(conf_->alpn.empty())
			LOG(WARNING) << "Client alpn is empty!";
	}
	engine_ = lsquic_engine_new(static_cast<unsigned>(flags), &engine_api);
	if(engine_ == nullptr)
	{
		LOG(ERROR) << "Create engine failed!";
		throw std::runtime_error("Create engine failed!");
	}
	LOG(INFO) << "Create engine success!";
}
ENGINE_BASE_TEMPLATE_DECL
void mqas::core::engine_base<E>::close_socket()
{
	if(socket_)
	{
		cxt.del_handle(socket_);
		socket_ = nullptr;
	}
}
ENGINE_BASE_TEMPLATE_DECL
void mqas::core::engine_base<E>::close_timer()
{
	if (proc_conns_timer_)
	{
		cxt.del_handle(proc_conns_timer_);
		proc_conns_timer_ = nullptr;
	}
}
ENGINE_BASE_TEMPLATE_DECL
void mqas::core::engine_base<E>::close_ssl_ctx()
{
	if (ssl_ctx_)
	{
		SSL_CTX_free(ssl_ctx_);
		ssl_ctx_ = nullptr;
	}
}

ENGINE_BASE_TEMPLATE_DECL
std::string mqas::core::engine_base<E>::load_config(const char* conf_file)
{
	std::stringstream ss;
#ifdef PF_ANDROID

#else
	std::ifstream file(conf_file);
	if (!file.is_open())
		throw std::runtime_error("Load config file failed!");
	ss << file.rdbuf();
	file.close();
#endif
	return ss.str();
}


//lsquic callback function implement
ENGINE_BASE_TEMPLATE_DECL
int mqas::core::engine_base<E>::lsquic_log_func(void* logger_ctx, const char* buf, size_t len)
{
	CLOG(ERROR, "lsquic") << buf;
	return 0;
}
ENGINE_BASE_TEMPLATE_DECL
lsquic_conn_ctx_t* mqas::core::engine_base<E>::on_new_conn_s(void* stream_if_ctx, lsquic_conn_t* lsquic_conn)
{
	return nullptr;
}
ENGINE_BASE_TEMPLATE_DECL
void mqas::core::engine_base<E>::on_conn_closed_s(lsquic_conn_t* lsquic_conn)
{
}
ENGINE_BASE_TEMPLATE_DECL
lsquic_stream_ctx_t* mqas::core::engine_base<E>::on_new_stream_s(void* stream_if_ctx, lsquic_stream_t* lsquic_stream)
{
	return nullptr;
}
ENGINE_BASE_TEMPLATE_DECL
void mqas::core::engine_base<E>::on_read_s(lsquic_stream_t* lsquic_stream, lsquic_stream_ctx_t* lsquic_stream_ctx)
{
}
ENGINE_BASE_TEMPLATE_DECL
void mqas::core::engine_base<E>::on_write_s(lsquic_stream_t* lsquic_stream, lsquic_stream_ctx_t* lsquic_stream_ctx)
{
}
ENGINE_BASE_TEMPLATE_DECL
void mqas::core::engine_base<E>::on_close_s(lsquic_stream_t* lsquic_stream, lsquic_stream_ctx_t* lsquic_stream_ctx)
{
}
ENGINE_BASE_TEMPLATE_DECL
int mqas::core::engine_base<E>::on_packets_out(void* packets_out_ctx, const lsquic_out_spec* out_spec,
	unsigned n_packets_out)
{
	return 0;
}
ENGINE_BASE_TEMPLATE_DECL
ssl_ctx_st* mqas::core::engine_base<E>::on_get_ssl_ctx(void* peer_ctx, const sockaddr* local)
{
	const auto engine = static_cast<engine_base*>(peer_ctx);
	return engine->ssl_ctx_;
}

ENGINE_BASE_TEMPLATE_DECL
int mqas::core::engine_base<E>::init_ssl(const char* cert_file, const char* key_file)
{
	LOG(INFO) << "initialize ssl ctx";
	int ret = 0;
	ssl_ctx_ = SSL_CTX_new(TLS_method());
	if (!ssl_ctx_)
	{
		LOG(ERROR) << "SSL_CTX_new failed";
		throw std::runtime_error("SSL_CTX_new failed\n");
	}
	SSL_CTX_set_min_proto_version(ssl_ctx_, TLS1_3_VERSION);
	SSL_CTX_set_max_proto_version(ssl_ctx_, TLS1_3_VERSION);
	SSL_CTX_set_default_verify_paths(ssl_ctx_);
	SSL_CTX_set_alpn_select_cb(ssl_ctx_, ssl_select_alpn_s, const_cast<char*>(conf_->alpn.c_str()));
	if ((ret = SSL_CTX_use_certificate_chain_file(ssl_ctx_, cert_file)) != 1)
	{
		LOG(ERROR) << "SSL_CTX_use_certificate_chain_file failed " << ret;
		throw std::runtime_error("SSL_CTX_use_certificate_chain_file failed");
	}
	if ((ret = SSL_CTX_use_PrivateKey_file(ssl_ctx_, key_file, SSL_FILETYPE_PEM)) != 1)
	{
		LOG(ERROR) << "SSL_CTX_use_PrivateKey_file failed " << ret;
		throw std::runtime_error("SSL_CTX_use_PrivateKey_file failed");
	}
	return 0;
}

#undef ENGINE_BASE_TEMPLATE_DECL