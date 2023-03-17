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
#include <mqas/io/exception.h>
#include <mqas/comm/string.h>

#ifdef PF_ANDROID
#endif

void MQAS_EXTERN settings_from_toml(::lsquic_engine_settings& s, const toml::value& v);
int MQAS_EXTERN ssl_select_alpn_s(SSL* ssl, const unsigned char** out, unsigned char* outlen,
	const unsigned char* in, unsigned inlen, void* arg);

#define ENGINE_BASE_TEMPLATE_DECL											\
template<typename E>														\
requires requires															\
{																			\
	requires std::is_default_constructible_v<E>;									\
	requires std::is_base_of_v<mqas::core::IEngine, E>;						\
}

ENGINE_BASE_TEMPLATE_DECL
mqas::core::engine_base<E>::engine_base(io::Context& c):cxt(c),socket_(nullptr),
proc_conns_timer_(nullptr),engine_flags_(EngineFlags::None),local_addr_({}),lsquic_logger_if_({}),
lsquic_engine_api_({}),lsquic_stream_if_({})
{}

ENGINE_BASE_TEMPLATE_DECL
mqas::core::engine_base<E>::engine_base(engine_base&& oth) noexcept : cxt(oth.cxt),
engine_flags_(oth.engine_flags_),local_addr_(oth.local_addr_),lsquic_logger_if_(oth.lsquic_logger_if_),
lsquic_engine_api_(oth.lsquic_engine_api_),lsquic_stream_if_(oth.lsquic_stream_if_)
{
	socket_ = oth.socket_;
	oth.socket_ = nullptr;

	proc_conns_timer_ = oth.proc_conns_timer_;
	oth.proc_conns_timer_ = nullptr;
}
ENGINE_BASE_TEMPLATE_DECL
void mqas::core::engine_base<E>::init(const char* conf_file,core::EngineFlags engine_flags)
{
	engine_flags_ = engine_flags;
	//parse config 
	const auto conf_data = toml::parse(conf_file);
	conf_ = std::make_shared<engine_config>(toml::find<engine_config>(conf_data, "engine_config"));

	init_extern_engine();
	//init logger
	init_logger();
	
	init_setting(conf_data);
	//init socket
	socket_ = cxt.make_handle<io::UdpSocket>();
	sockaddr addr{};
	io::Ip::str2addr_ipv4(conf_->bind_ip.c_str(),conf_->port, addr);
	socket_->bind(addr,UV_UDP_REUSEADDR);
	socket_->get_sock_addr(this->local_addr_);
	engine_extern_->on_init_socket(socket_);
	// init timer
	proc_conns_timer_ = cxt.make_handle<io::Timer>();
	
	//init ssl
	if (contain<uint32_t>(engine_flags, EngineFlags::Server) && !conf_->ssl_cert_path.empty() && !conf_->ssl_key_path.empty())
		init_ssl(conf_->ssl_cert_path.c_str(),conf_->ssl_key_path.c_str());
	init_lsquic();
}

ENGINE_BASE_TEMPLATE_DECL
void mqas::core::engine_base<E>::init_setting(const toml::value& conf_data)
{
	::lsquic_engine_init_settings(&conf_->lsquic_settings, static_cast<unsigned>(engine_flags_));
	if (conf_data.contains("lsquic_settings"))
		settings_from_toml(conf_->lsquic_settings, conf_data.at("lsquic_settings"));

	char errbuf[512] = {0};
	if (0 != ::lsquic_engine_check_settings(&conf_->lsquic_settings, static_cast<unsigned>(engine_flags_),errbuf, sizeof(errbuf)))
	{
		LOG(ERROR) << "invalid settings: " << errbuf;
		throw std::runtime_error("Invalid settings look log file!");
	}
}

ENGINE_BASE_TEMPLATE_DECL
void mqas::core::engine_base<E>::init_extern_engine()
{
	engine_extern_ = std::make_shared<E>();
	engine_extern_->init(static_cast<void*>(this));
}

ENGINE_BASE_TEMPLATE_DECL
void mqas::core::engine_base<E>::init_logger() const
{
	mqas::log::init("default",conf_->log_config,std::nullopt);
	const auto lsquic_log = el::Loggers::getLogger("lsquic");
	el::Configurations c;
	c.setFromBase(el::Loggers::getLogger("default")->configurations());
	auto fmt = c.get(el::Level::Global,el::ConfigurationType::Format)->value();
	bool erase_succ = mqas::comm::erase_substr(fmt,"[%level]");
	if(!erase_succ) erase_succ = mqas::comm::erase_substr(fmt, "[%levshort]");
	if(erase_succ)
		c.set(el::Level::Global,el::ConfigurationType::Format,fmt);
	el::Loggers::reconfigureLogger(lsquic_log, c);
	engine_extern_->on_init_logger();
}
ENGINE_BASE_TEMPLATE_DECL
void mqas::core::engine_base<E>::init_lsquic() noexcept(false)
{
	lsquic_logger_if_ = { lsquic_log_func, };

	lsquic_logger_init(&lsquic_logger_if_, this, LLTS_NONE);
	lsquic_set_log_level(conf_->log_level.c_str());
	
	lsquic_stream_if_.on_new_conn = on_new_conn_s;
	lsquic_stream_if_.on_conn_closed = on_conn_closed_s;
	lsquic_stream_if_.on_new_stream = on_new_stream_s;
	lsquic_stream_if_.on_read = on_read_s;
	lsquic_stream_if_.on_write = on_write_s;
	lsquic_stream_if_.on_close = on_close_s;
	lsquic_stream_if_.on_goaway_received = on_goaway_received;
	lsquic_stream_if_.on_dg_write = on_dg_write;
	lsquic_stream_if_.on_datagram = on_datagram;
	lsquic_stream_if_.on_hsk_done = on_hsk_done;
	lsquic_stream_if_.on_new_token = on_new_token;
	lsquic_stream_if_.on_reset = on_reset;
	lsquic_stream_if_.on_conncloseframe_received = on_conncloseframe_received;
	
	lsquic_engine_api_.ea_settings = &conf_->lsquic_settings;
	lsquic_engine_api_.ea_packets_out = on_packets_out;
	lsquic_engine_api_.ea_packets_out_ctx = socket_;
	lsquic_engine_api_.ea_stream_if = &lsquic_stream_if_;
	lsquic_engine_api_.ea_stream_if_ctx = engine_extern_.get();
	if(contain<uint32_t>(engine_flags_,EngineFlags::Server))
		lsquic_engine_api_.ea_get_ssl_ctx = on_get_ssl_ctx;
	if(engine_flags_ == EngineFlags::None)
	{
		lsquic_engine_api_.ea_alpn = conf_->alpn.c_str();
		if(conf_->alpn.empty())
			LOG(WARNING) << "Client alpn is empty!";
	}
	engine_extern_->on_new_lsquic_engine(lsquic_engine_api_, engine_flags_);
	engine_ = lsquic_engine_new(static_cast<unsigned>(engine_flags_), &lsquic_engine_api_);
	if(engine_ == nullptr)
	{
		LOG(ERROR) << "Create engine failed!";
		throw std::runtime_error("Create engine failed!");
	}
	LOG(INFO) << "Create engine success!";
}

ENGINE_BASE_TEMPLATE_DECL
void mqas::core::engine_base<E>::start_recv() const
{
	socket_->recv_start([this](io::UdpSocket* sock,const std::optional<std::span<uint8_t>>& buf,ssize_t nread,const sockaddr* addr,unsigned flags)
	{
		if (nread == 0 || addr == nullptr)
			return;
		if (nread < 0) {
			// there seems to be no way to get an error code here (none of the udp tests do)
			LOG(ERROR) << "udp recv error unexpected nread = " << nread;
			return;
		}
		if(engine_extern_->on_recv(buf,nread,addr,flags))
		{
			const int ret = ::lsquic_engine_packet_in(engine_, reinterpret_cast<const unsigned char*>(buf->data()), nread,
				&this->local_addr_, addr, static_cast<void*>(const_cast<engine_base*>(this)), 0);
			//LOG(INFO) << "lsquic_engine_packet_in ret = " << ret;
			this->process_conns();
		}
	});
}

ENGINE_BASE_TEMPLATE_DECL
::lsquic_conn_t* mqas::core::engine_base<E>::connect(const ::sockaddr& addr, ::lsquic_version ver, const char* hostname, unsigned short base_plpmtu,
	const unsigned char* sess_resume, size_t sess_resume_len,
	const unsigned char* token, size_t token_sz)
{
	auto conn = ::lsquic_engine_connect(engine_, ver,&local_addr_, &addr, nullptr, reinterpret_cast<::lsquic_conn_ctx*>(engine_extern_.get()), hostname, base_plpmtu, sess_resume, sess_resume_len, token, token_sz);
#if !NDEBUG
    LOG(INFO) << "connect conn = " <<  reinterpret_cast<size_t>(conn);
#endif
	process_conns();
	return conn;
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
	const auto engine = static_cast<E*>(stream_if_ctx);
	engine->on_new_conn(stream_if_ctx,lsquic_conn);
	return reinterpret_cast<lsquic_conn_ctx_t*>(engine);
}
ENGINE_BASE_TEMPLATE_DECL
void mqas::core::engine_base<E>::on_conn_closed_s(lsquic_conn_t* lsquic_conn)
{
	const auto engine = reinterpret_cast<E*>(::lsquic_conn_get_ctx(lsquic_conn));
	engine->on_conn_closed(lsquic_conn);
}
ENGINE_BASE_TEMPLATE_DECL
lsquic_stream_ctx_t* mqas::core::engine_base<E>::on_new_stream_s(void* stream_if_ctx, lsquic_stream_t* lsquic_stream)
{
	const auto engine = static_cast<E*>(stream_if_ctx);
	engine->on_new_stream(stream_if_ctx, lsquic_stream);
	return reinterpret_cast<lsquic_stream_ctx_t*>(engine);
}
ENGINE_BASE_TEMPLATE_DECL
void mqas::core::engine_base<E>::on_read_s(lsquic_stream_t* lsquic_stream, lsquic_stream_ctx_t* lsquic_stream_ctx)
{
	const auto engine = reinterpret_cast<E*>(lsquic_stream_ctx);
	engine->on_read(lsquic_stream,lsquic_stream_ctx);
}
ENGINE_BASE_TEMPLATE_DECL
void mqas::core::engine_base<E>::on_write_s(lsquic_stream_t* lsquic_stream, lsquic_stream_ctx_t* lsquic_stream_ctx)
{
	const auto engine = reinterpret_cast<E*>(lsquic_stream_ctx);
	engine->on_write(lsquic_stream, lsquic_stream_ctx);
}
ENGINE_BASE_TEMPLATE_DECL
void mqas::core::engine_base<E>::on_close_s(lsquic_stream_t* lsquic_stream, lsquic_stream_ctx_t* lsquic_stream_ctx)
{
	const auto engine = reinterpret_cast<E*>(lsquic_stream_ctx);
	engine->on_close(lsquic_stream, lsquic_stream_ctx);
}

ENGINE_BASE_TEMPLATE_DECL
void mqas::core::engine_base<E>::process_conns() const
{
	proc_conns_timer_->stop();
	int diff;
	::lsquic_engine_process_conns(engine_);
	if (::lsquic_engine_earliest_adv_tick(engine_, &diff)) {
		int timeout;
		if (diff >= LSQUIC_DF_CLOCK_GRANULARITY)
			/* Expected case: convert to millisecond */
			timeout = diff / 1000;
		else if (diff <= 0)
			/* It should not happen often that the next tick is in the past
			 * as we just processed connections.  Avoid a busy loop by
			 * scheduling an event:
			 */
			timeout = 0.0;
		else
			/* Round up to granularity */
			timeout = LSQUIC_DF_CLOCK_GRANULARITY / 1000;
		proc_conns_timer_->start([this](io::Timer* t)
		{
			this->process_conns();
		},timeout,0);
	}
}
ENGINE_BASE_TEMPLATE_DECL
void mqas::core::engine_base<E>::process_conns_lazy() const
{
    proc_conns_timer_->stop();
    proc_conns_timer_->start([this](io::Timer* t)
        {
            this->process_conns();
        },0,0);
}
ENGINE_BASE_TEMPLATE_DECL
std::shared_ptr<E> mqas::core::engine_base<E>::get_engine() const
{
	return engine_extern_;
}
ENGINE_BASE_TEMPLATE_DECL
int mqas::core::engine_base<E>::on_packets_out(void* packets_out_ctx, const lsquic_out_spec* out_spec,
	unsigned n_packets_out)
{
	const auto sock = static_cast<io::UdpSocket*>(packets_out_ctx);

	std::vector<std::span<uint8_t>> bufs;
	unsigned succ_num = n_packets_out;
	for (unsigned n = 0; n < n_packets_out; ++n)
	{
		if (bufs.size() < out_spec[n].iovlen)
			bufs.resize(out_spec[n].iovlen);
		for (unsigned i = 0; i < out_spec[n].iovlen; ++i)
		{
			bufs[i] = std::span<uint8_t>(static_cast<uint8_t *>(out_spec[n].iov[i].iov_base),out_spec[n].iov[i].iov_len);
		}
		try{
            sock->send(bufs,*out_spec[n].dest_sa,[](io::UdpSocket* s,int status){
                if(status != 0) LOG(ERROR) << "packets_out send failed status = " << status;
            });
			//sock->try_send(bufs, *out_spec[n].dest_sa);
		}catch (io::Exception& e)
		{
			--succ_num;
			LOG(ERROR) << "packets_out send failed exception = " << e.what();
		}
	}
	return static_cast<int>(succ_num);
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
	//LOG(INFO) << "initialize ssl ctx";
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

//lsquic optional stream callback
ENGINE_BASE_TEMPLATE_DECL
void mqas::core::engine_base<E>::on_goaway_received(lsquic_conn_t* c)
{
	const auto engine = reinterpret_cast<E*>(::lsquic_conn_get_ctx(c));
	engine->on_goaway_received(c);
}
ENGINE_BASE_TEMPLATE_DECL
ssize_t mqas::core::engine_base<E>::on_dg_write(lsquic_conn_t* c, void* buf, size_t buf_sz)
{
	const auto engine = reinterpret_cast<E*>(::lsquic_conn_get_ctx(c));
	return engine->on_dg_write(c,buf,buf_sz);
}
ENGINE_BASE_TEMPLATE_DECL
void mqas::core::engine_base<E>::on_datagram(lsquic_conn_t* c, const void* buf, size_t buf_sz)
{
	const auto engine = reinterpret_cast<E*>(::lsquic_conn_get_ctx(c));
	engine->on_datagram(c, buf, buf_sz);
}
ENGINE_BASE_TEMPLATE_DECL
void mqas::core::engine_base<E>::on_hsk_done(lsquic_conn_t* c, enum lsquic_hsk_status s)
{
	const auto engine = reinterpret_cast<E*>(::lsquic_conn_get_ctx(c));
	engine->on_hsk_done(c, s);
}
ENGINE_BASE_TEMPLATE_DECL
void mqas::core::engine_base<E>::on_new_token(lsquic_conn_t* c, const unsigned char* token, size_t token_size)
{
	const auto engine = reinterpret_cast<E*>(::lsquic_conn_get_ctx(c));
	engine->on_new_token(c,token, token_size);
}
ENGINE_BASE_TEMPLATE_DECL
void mqas::core::engine_base<E>::on_reset(lsquic_stream_t* s, lsquic_stream_ctx_t* h, int how)
{
	const auto engine = reinterpret_cast<E*>(h);
	engine->on_reset(s,h,how);
}
ENGINE_BASE_TEMPLATE_DECL
void mqas::core::engine_base<E>::on_conncloseframe_received(lsquic_conn_t* c, int app_error, uint64_t error_code, const char* reason, int reason_len)
{
	const auto engine = reinterpret_cast<E*>(::lsquic_conn_get_ctx(c));
	engine->on_conncloseframe_received(c,app_error,error_code,reason, reason_len);
}

#undef ENGINE_BASE_TEMPLATE_DECL