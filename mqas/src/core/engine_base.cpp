#include <fstream>
#include<mqas/core/engine_base.h>
#include <sstream>
#include <stdexcept>
#include <mqas/io/udp.h>
#include <mqas/io/timer.h>
#include "easylogging++.h"
#include <lsquic.h>
#include <mqas/log.h>
#include <mqas/comm/macro.h>

#ifdef PF_ANDROID
#endif
mqas::core::engine_base::engine_base(io::Context& c):cxt(c),socket_(nullptr)
,proc_conns_timer_(nullptr)
{}

mqas::core::engine_base::engine_base(engine_base&& oth) noexcept : cxt(oth.cxt)
{
	socket_ = oth.socket_;
	oth.socket_ = nullptr;

	proc_conns_timer_ = oth.proc_conns_timer_;
	oth.proc_conns_timer_ = nullptr;
}

void mqas::core::engine_base::init(const char* conf_file,core::EngineFlags engine_flags)
{
	//parse config 
	const auto conf_data = toml::parse(conf_file);
	conf_ = std::make_shared<engine_config>(toml::find<engine_config>(conf_data, "engine_config"));
	
	//init socket
	socket_ = cxt.make_handle<io::UdpSocket>();
	sockaddr addr{};
	io::UdpSocket::str2addr_ipv4(conf_->bind_ip.c_str(),conf_->port, addr);
	socket_->bind(addr,UV_UDP_REUSEADDR);
	// init timer
	proc_conns_timer_ = cxt.make_handle<io::Timer>();
	//init logger
	//register lsquic logger
	el::Loggers::getLogger("lsquic");
	mqas::log::init(conf_->log_config);

	init_lsquic(engine_flags);
}

void mqas::core::engine_base::init_lsquic(core::EngineFlags flags) noexcept(false)
{
	constexpr ::lsquic_logger_if logger_if = { lsquic_log_func, };

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

void mqas::core::engine_base::close_socket()
{
	if(socket_)
	{
		cxt.del_handle(socket_);
		socket_ = nullptr;
	}
}

void mqas::core::engine_base::close_timer()
{
	if (proc_conns_timer_)
	{
		cxt.del_handle(proc_conns_timer_);
		proc_conns_timer_ = nullptr;
	}
}

mqas::core::engine_base::~engine_base()
{
	close_socket();
	close_timer();
}

std::string mqas::core::engine_base::load_config(const char* conf_file)
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
//engine config deserialize
mqas::core::engine_config toml::from<mqas::core::engine_config>::from_toml(const value& v)
{
	mqas::core::engine_config f;
	f.bind_ip =		find_or<std::string		>(v,		"bind_ip",		"0.0.0.0");
	f.port =		find_or<short			>(v,		"port",		8083);
	f.log_level =	find_or<std::string		>(v,		"log_level",	"warning");
	f.log_path=		find_or<std::string		>(v,		"log_path",	"log.txt");
	f.log_config =	find_or<std::string		>(v,		"log_config", "");
	f.alpn =		find_or<std::string		>(v,		"alpn",		"");
	return f;
}
//lsquic callback function implement
int mqas::core::engine_base::lsquic_log_func(void* logger_ctx, const char* buf, size_t len)
{
	//auto* p = static_cast<engine_base*>(logger_ctx);
	CLOG(ERROR, "lsquic") << buf;
	return 0;
}

lsquic_conn_ctx_t* mqas::core::engine_base::on_new_conn_s(void* stream_if_ctx, lsquic_conn_t* lsquic_conn)
{
	return nullptr;
}

void mqas::core::engine_base::on_conn_closed_s(lsquic_conn_t* lsquic_conn)
{
}

lsquic_stream_ctx_t* mqas::core::engine_base::on_new_stream_s(void* stream_if_ctx, lsquic_stream_t* lsquic_stream)
{
	return nullptr;
}

void mqas::core::engine_base::on_read_s(lsquic_stream_t* lsquic_stream, lsquic_stream_ctx_t* lsquic_stream_ctx)
{
}

void mqas::core::engine_base::on_write_s(lsquic_stream_t* lsquic_stream, lsquic_stream_ctx_t* lsquic_stream_ctx)
{
}

void mqas::core::engine_base::on_close_s(lsquic_stream_t* lsquic_stream, lsquic_stream_ctx_t* lsquic_stream_ctx)
{
}

int mqas::core::engine_base::on_packets_out(void* packets_out_ctx, const lsquic_out_spec* out_spec,
	unsigned n_packets_out)
{
	return 0;
}

ssl_ctx_st* mqas::core::engine_base::on_get_ssl_ctx(void* peer_ctx, const sockaddr* local)
{
	return nullptr;
}
