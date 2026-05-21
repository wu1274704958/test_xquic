#pragma once

#include <fstream>
#include <sstream>
#include <stdexcept>
#include <mqas/io/udp.h>
#include <mqas/io/timer.h>
#include <mqas/io/idle.h>
#include "easylogging++.h"
#include <lsquic.h>
#include <mqas/log.h>
#include <mqas/comm/macro.h>
#include <mqas/io/ip.h>
#include <mqas/io/exception.h>
#include <mqas/comm/string.h>

#ifdef PF_ANDROID
#endif

int MQAS_EXTERN ssl_select_alpn_s(SSL* ssl, const unsigned char** out, unsigned char* outlen,
	const unsigned char* in, unsigned inlen, void* arg);

#define ENGINE_BASE_TEMPLATE_DECL											\
template<typename E,typename ED,typename SC>								\
requires requires															\
{																			\
	requires mqas::core::IsVaildEngineDriver<ED>;							\
	requires std::is_default_constructible_v<E>;							\
	requires std::is_base_of_v<mqas::core::IEngine, E>;						\
	requires mqas::core::IsVaildSocket<SC>;									\
}

ENGINE_BASE_TEMPLATE_DECL
mqas::core::engine_base<E,ED,SC>::engine_base(io::Context& c):io_cxt(c),socket_(nullptr),
proc_conns_timer_(nullptr),engine_flags_(EngineFlags::None),local_addr_({}),lsquic_logger_if_({}),
lsquic_engine_api_({}),lsquic_stream_if_({})
{}

ENGINE_BASE_TEMPLATE_DECL
mqas::core::engine_base<E,ED,SC>::engine_base(engine_base&& oth) noexcept : io_cxt(oth.io_cxt),
engine_flags_(oth.engine_flags_),local_addr_(oth.local_addr_),lsquic_logger_if_(oth.lsquic_logger_if_),
lsquic_engine_api_(oth.lsquic_engine_api_),lsquic_stream_if_(oth.lsquic_stream_if_)
{
	socket_ = oth.socket_;
	oth.socket_ = nullptr;

	proc_conns_timer_ = oth.proc_conns_timer_;
	oth.proc_conns_timer_ = nullptr;
}

ENGINE_BASE_TEMPLATE_DECL
void mqas::core::engine_base<E,ED,SC>::init_socket(std::shared_ptr<io::UdpSocket> sock)
{
	if (sock == nullptr)
	{
		socket_ = io_cxt.make_shared<io::UdpSocket>();
		sockaddr addr{};
		io::Ip::str2addr_ipv4(conf_->bind_ip.c_str(), conf_->port, addr);
		socket_->bind(addr, UV_UDP_REUSEADDR);
	}
	else
		socket_ = std::move(sock);
	socket_->get_sock_addr(this->local_addr_);
}

ENGINE_BASE_TEMPLATE_DECL
void mqas::core::engine_base<E,ED,SC>::init(const char* conf_file, core::EngineFlags engine_flags, std::shared_ptr<io::UdpSocket> socket)
{
	//parse config 
	init_config(conf_file);
	
	init_lsquic();

	//init logger
	init_logger();

	engine_flags_ = engine_flags;

	engine_extern_ = std::make_shared<E>();

	//init socket
	init_socket(std::move(socket));

	init_engine_core();

	init_context();

	engine_extern_->init(context);
	
	// init timer
	init_timer();
}

ENGINE_BASE_TEMPLATE_DECL
void mqas::core::engine_base<E,ED,SC>::init_config(const char* conf_file)
{
	conf_origin_ = std::make_shared<toml::value>(toml::parse(conf_file));
	conf_ = std::make_shared<engine_config>(toml::find<engine_config>(*conf_origin_, "engine_config"));
}

ENGINE_BASE_TEMPLATE_DECL
bool mqas::core::engine_base<E,ED,SC>::has_engine_setting() const
{
	return conf_origin_ != nullptr && conf_origin_->contains("lsquic_settings");
}
ENGINE_BASE_TEMPLATE_DECL
void mqas::core::engine_base<E,ED,SC>::init_timer()
{
	proc_conns_timer_ = io_cxt.make_handle<io::Timer>();
}
ENGINE_BASE_TEMPLATE_DECL
void mqas::core::engine_base<E,ED,SC>::init_engine_core()
{
	lsquic_engine_api_.ea_packets_out_ctx = socket_.get();
	
	lsquic_engine_api_.ea_packets_out = on_packets_out;
	lsquic_engine_api_.ea_stream_if = &lsquic_stream_if_;
	lsquic_engine_api_.ea_stream_if_ctx = engine_extern_.get();

	if (has_engine_setting())
	{
		::lsquic_engine_init_settings(&conf_->lsquic_settings, static_cast<unsigned>(engine_flags_));
		engine_driver::settings_from_toml(conf_->lsquic_settings, conf_origin_->at("lsquic_settings"),
			comm::contain<uint32_t>(engine_flags_, EngineFlags::Server));

		lsquic_engine_api_.ea_settings = &conf_->lsquic_settings;
	}

	if (comm::contain<uint32_t>(engine_flags_, EngineFlags::Server))
	{
		lsquic_engine_api_.ea_get_ssl_ctx = on_get_ssl_ctx;
		ssl_ctx_ = ED::instance()->get_ssl_or_generate(conf_->ssl_cert_path, conf_->ssl_key_path,conf_->alpn);
	}
	if (engine_flags_ == EngineFlags::None)
	{
		lsquic_engine_api_.ea_alpn = conf_->alpn.c_str();
		if (conf_->alpn.empty())
			LOG(WARNING) << "Client alpn is empty!";
	}

	if (has_engine_setting())
	{
		char errbuf[512] = { 0 };
		if (0 != ::lsquic_engine_check_settings(&conf_->lsquic_settings,
			static_cast<unsigned>(engine_flags_), errbuf, sizeof(errbuf)))
		{
			LOG(ERROR) << "invalid settings: " << errbuf;
			throw std::runtime_error("Invalid settings look log file!");
		}
	}

	engine_extern_->on_new_lsquic_engine(lsquic_engine_api_, engine_flags_);
	engine_ = lsquic_engine_new(static_cast<unsigned>(engine_flags_), &lsquic_engine_api_);
	if (engine_ == nullptr)
	{
		LOG(ERROR) << "Create engine failed!";
		throw std::runtime_error("Create engine failed!");
	}
	LOG(INFO) << "Create engine success!";

	engine_extern_->on_init_config(conf_origin_);
	engine_extern_->on_init_engine_config(conf_);
	if constexpr(std::is_same_v<SC,io::UdpSocket>)
	{
		engine_extern_->on_init_socket(socket_);
	}
}

ENGINE_BASE_TEMPLATE_DECL
void mqas::core::engine_base<E,ED,SC>::init_logger() const
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
void mqas::core::engine_base<E,ED,SC>::init_context()
{
	context = std::make_shared<engine_cxt>(io_cxt,local_addr_);
	context->engine_core = engine_;
	context->engine_flags = engine_flags_;
	context->process_conns = std::bind(&engine_base::process_conns, this);
	context->process_conns_lazy = std::bind(&engine_base::process_conns_lazy, this);
}

ENGINE_BASE_TEMPLATE_DECL
void mqas::core::engine_base<E,ED,SC>::init_lsquic() noexcept(false)
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
}

ENGINE_BASE_TEMPLATE_DECL
void mqas::core::engine_base<E,ED,SC>::start_recv()
{
	if(recv_connection_.connected())
		return;
	socket_->recv_start();
	recv_connection_ = socket_->on_recv_signal.connect([this](io::UdpSocket* sock, const std::optional<std::span<uint8_t>>& buf, ssize_t nread, const sockaddr* addr, unsigned flags)
	{
		if (nread == 0 || addr == nullptr)
			return;
		if (nread < 0) {
			// there seems to be no way to get an error code here (none of the udp tests do)
			LOG(ERROR) << "udp recv error unexpected nread = " << nread;
			return;
		}
		if (engine_extern_->on_recv(buf, nread, addr, flags))
		{
			const int ret = ::lsquic_engine_packet_in(engine_, reinterpret_cast<const unsigned char*>(buf->data()), nread,
				&this->local_addr_, addr, (void*)&(_peer_context_mgr.get_or_create(addr,this)), 0);
			//LOG(INFO) << "lsquic_engine_packet_in ret = " << ret;
			this->process_conns();
		}
	});
}

ENGINE_BASE_TEMPLATE_DECL
void mqas::core::engine_base<E,ED,SC>::close_socket()
{
	
}
ENGINE_BASE_TEMPLATE_DECL
void mqas::core::engine_base<E,ED,SC>::close_timer()
{
	if (proc_conns_timer_)
	{
		proc_conns_timer_->stop();
		io_cxt.del_handle(proc_conns_timer_);
		proc_conns_timer_ = nullptr;
	}
}
ENGINE_BASE_TEMPLATE_DECL
void mqas::core::engine_base<E,ED,SC>::close_ssl_ctx()
{
	ssl_ctx_ = ED::instance()->destroy_ssl_ctx(ssl_ctx_);
}

ENGINE_BASE_TEMPLATE_DECL
void mqas::core::engine_base<E,ED,SC>::close()
{
	if (engine_extern_)
		engine_extern_->close();
	while (engine_extern_ && engine_extern_->connect_count() > 0)
		io_cxt.run(mqas::io::Context::RunMode::ONCE);
	engine_extern_.reset();
}

ENGINE_BASE_TEMPLATE_DECL
mqas::core::engine_base<E,ED,SC>::~engine_base()
{
	close();
	close_ssl_ctx();
	if (engine_)
		::lsquic_engine_destroy(engine_);
	if (recv_connection_.connected())
		recv_connection_.disconnect();
	close_socket();
	close_timer();
}

ENGINE_BASE_TEMPLATE_DECL
std::string mqas::core::engine_base<E,ED,SC>::load_config(const char* conf_file)
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

ENGINE_BASE_TEMPLATE_DECL
E* mqas::core::engine_base<E,ED,SC>::get_engine_by_cxt(void* cxt)
{
	if(cxt == nullptr) return nullptr;
	auto engine = reinterpret_cast<E*>(cxt);
	if(engine == nullptr || !IEngine::is_valid(engine))
		return nullptr;
	return engine;
}


//lsquic callback function implement
ENGINE_BASE_TEMPLATE_DECL
int mqas::core::engine_base<E,ED,SC>::lsquic_log_func(void* logger_ctx, const char* buf, size_t len)
{
	CLOG(ERROR, "lsquic") << buf;
	return 0;
}

ENGINE_BASE_TEMPLATE_DECL
lsquic_conn_ctx_t* mqas::core::engine_base<E,ED,SC>::on_new_conn_s(void* stream_if_ctx, lsquic_conn_t* lsquic_conn)
{
	const auto engine = get_engine_by_cxt(stream_if_ctx);
	if(engine == nullptr)
	{
		CLOG(ERROR, "lsquic") << "on_new_conn_s get_engine_by_cxt is null! cxt = " << stream_if_ctx << " conn = " << lsquic_conn;
		return nullptr;
	}
	engine->on_new_conn(stream_if_ctx,lsquic_conn);
	return reinterpret_cast<lsquic_conn_ctx_t*>(engine);
}

ENGINE_BASE_TEMPLATE_DECL
void mqas::core::engine_base<E,ED,SC>::on_conn_closed_s(lsquic_conn_t* lsquic_conn)
{
	const auto engine = get_engine_by_cxt(::lsquic_conn_get_ctx(lsquic_conn));
	if(engine == nullptr)
	{
		CLOG(ERROR, "lsquic") << "on_conn_closed_s get_engine_by_cxt is null! conn = " << lsquic_conn;
		return;
	}
	engine->on_conn_closed(lsquic_conn);
	::lsquic_conn_set_ctx(lsquic_conn, NULL);
}
ENGINE_BASE_TEMPLATE_DECL
lsquic_stream_ctx_t* mqas::core::engine_base<E,ED,SC>::on_new_stream_s(void* stream_if_ctx, lsquic_stream_t* lsquic_stream)
{
	const auto engine = get_engine_by_cxt(stream_if_ctx);
	if(engine == nullptr)
	{
		CLOG(ERROR, "lsquic") << "on_new_stream_s get_engine_by_cxt is null! cxt = " << stream_if_ctx << " stream = " << lsquic_stream;
		return nullptr;
	}
	engine->on_new_stream(stream_if_ctx, lsquic_stream);
	return reinterpret_cast<lsquic_stream_ctx_t*>(engine);
}
ENGINE_BASE_TEMPLATE_DECL
void mqas::core::engine_base<E,ED,SC>::on_read_s(lsquic_stream_t* lsquic_stream, lsquic_stream_ctx_t* lsquic_stream_ctx)
{
	const auto engine = get_engine_by_cxt(lsquic_stream_ctx);
	if(engine == nullptr)
	{
		CLOG(ERROR, "lsquic") << "on_read_s get_engine_by_cxt is null! stream = " << lsquic_stream << " cxt = " << lsquic_stream_ctx;
		return;
	}
	engine->on_read(lsquic_stream,lsquic_stream_ctx);
}
ENGINE_BASE_TEMPLATE_DECL
void mqas::core::engine_base<E,ED,SC>::on_write_s(lsquic_stream_t* lsquic_stream, lsquic_stream_ctx_t* lsquic_stream_ctx)
{
	const auto engine = get_engine_by_cxt(lsquic_stream_ctx);
	if(engine == nullptr)
	{
		CLOG(ERROR, "lsquic") << "on_write_s get_engine_by_cxt is null! stream = " << lsquic_stream << " cxt = " << lsquic_stream_ctx;
		return;
	}
	engine->on_write(lsquic_stream, lsquic_stream_ctx);
}
ENGINE_BASE_TEMPLATE_DECL
void mqas::core::engine_base<E,ED,SC>::on_close_s(lsquic_stream_t* lsquic_stream, lsquic_stream_ctx_t* lsquic_stream_ctx)
{
	const auto engine = get_engine_by_cxt(lsquic_stream_ctx);
	if(engine == nullptr)
	{
		CLOG(ERROR, "lsquic") << "on_close_s get_engine_by_cxt is null! stream = " << lsquic_stream << " cxt = " << lsquic_stream_ctx;
		return;
	}
	engine->on_close(lsquic_stream, lsquic_stream_ctx);
}

ENGINE_BASE_TEMPLATE_DECL
void mqas::core::engine_base<E,ED,SC>::process_conns() const
{
	proc_conns_timer_->stop();
	if (engine_ == nullptr) return;
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
void mqas::core::engine_base<E,ED,SC>::process_conns_lazy() const
{
    proc_conns_timer_->stop();
    proc_conns_timer_->start([this](io::Timer* t)
        {
            this->process_conns();
        },0,0);
}
ENGINE_BASE_TEMPLATE_DECL
std::shared_ptr<E> mqas::core::engine_base<E,ED,SC>::get_engine() const
{
	return engine_extern_;
}
ENGINE_BASE_TEMPLATE_DECL
int mqas::core::engine_base<E,ED,SC>::on_packets_out(void* packets_out_ctx, const lsquic_out_spec* out_spec,
	unsigned n_packets_out)
{
	const auto sock = static_cast<SC*>(packets_out_ctx);

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
            //sock->send(bufs,*out_spec[n].dest_sa,[](io::UdpSocket* s,int status){
            //    if(status != 0) LOG(ERROR) << "packets_out send failed status = " << status;
            //});
			sock->try_send(bufs, *out_spec[n].dest_sa);
		}catch (io::Exception& e)
		{
			--succ_num;
			LOG(ERROR) << "packets_out send failed exception = " << e.what();
		}
	}
	return static_cast<int>(succ_num);
}
ENGINE_BASE_TEMPLATE_DECL
ssl_ctx_st* mqas::core::engine_base<E,ED,SC>::on_get_ssl_ctx(void* peer_ctx, const sockaddr* local)
{
	auto ptr = static_cast<peer_context<engine_base<E, ED>>*>(peer_ctx);
	return ptr->engine->ssl_ctx_;
}

//lsquic optional stream callback
ENGINE_BASE_TEMPLATE_DECL
void mqas::core::engine_base<E,ED,SC>::on_goaway_received(lsquic_conn_t* c)
{
	const auto cxt = ::lsquic_conn_get_ctx(c);
	const auto engine = get_engine_by_cxt(cxt);
	if(engine == nullptr)
	{
		CLOG(ERROR, "lsquic") << "on_goaway_received get_engine_by_cxt is null! conn = " << c;
		return;
	}
	engine->on_goaway_received(c);
}
ENGINE_BASE_TEMPLATE_DECL
ssize_t mqas::core::engine_base<E,ED,SC>::on_dg_write(lsquic_conn_t* c, void* buf, size_t buf_sz)
{
	const auto cxt = ::lsquic_conn_get_ctx(c);
	const auto engine = get_engine_by_cxt(cxt);
	if(engine == nullptr)
	{
		CLOG(ERROR, "lsquic") << "on_dg_write get_engine_by_cxt is null! conn = " << c;
		return 0;
	}
	return engine->on_dg_write(c,buf,buf_sz);
}
ENGINE_BASE_TEMPLATE_DECL
void mqas::core::engine_base<E,ED,SC>::on_datagram(lsquic_conn_t* c, const void* buf, size_t buf_sz)
{
	const auto cxt = ::lsquic_conn_get_ctx(c);
	const auto engine = get_engine_by_cxt(cxt);
	if(engine == nullptr)
	{
		CLOG(ERROR, "lsquic") << "on_datagram get_engine_by_cxt is null! conn = " << c;
		return;
	}
	engine->on_datagram(c, buf, buf_sz);
}
ENGINE_BASE_TEMPLATE_DECL
void mqas::core::engine_base<E,ED,SC>::on_hsk_done(lsquic_conn_t* c, enum lsquic_hsk_status s)
{
	const auto cxt = ::lsquic_conn_get_ctx(c);
	const auto engine = get_engine_by_cxt(cxt);
	if(engine == nullptr)
	{
		CLOG(DEBUG, "lsquic") << "on_hsk_done get_engine_by_cxt is null! conn = " << c;
		return;
	}
	engine->on_hsk_done(c, s);
}
ENGINE_BASE_TEMPLATE_DECL
void mqas::core::engine_base<E,ED,SC>::on_new_token(lsquic_conn_t* c, const unsigned char* token, size_t token_size)
{
	const auto cxt = ::lsquic_conn_get_ctx(c);
	const auto engine = get_engine_by_cxt(cxt);
	if(engine == nullptr)
	{
		CLOG(DEBUG, "lsquic") << "on_new_token get_engine_by_cxt is null! conn = " << c;
		return;
	}
	engine->on_new_token(c,token, token_size);
}
ENGINE_BASE_TEMPLATE_DECL
void mqas::core::engine_base<E,ED,SC>::on_reset(lsquic_stream_t* s, lsquic_stream_ctx_t* h, int how)
{
	const auto engine = get_engine_by_cxt(h);
	if(engine == nullptr)
	{
		CLOG(ERROR, "lsquic") << "on_reset get_engine_by_cxt is null! stream = " << s;
		return;
	}
	engine->on_reset(s,h,how);
}
ENGINE_BASE_TEMPLATE_DECL
void mqas::core::engine_base<E,ED,SC>::on_conncloseframe_received(lsquic_conn_t* c, int app_error, uint64_t error_code, const char* reason, int reason_len)
{
	const auto cxt = ::lsquic_conn_get_ctx(c);
	const auto engine = get_engine_by_cxt(cxt);
	if(engine == nullptr)
	{
		CLOG(DEBUG, "lsquic") << "on_conncloseframe_received get_engine_by_cxt is null! conn = " << c;
		return;
	}
	engine->on_conncloseframe_received(c,app_error,error_code,reason, reason_len);
}

#undef ENGINE_BASE_TEMPLATE_DECL