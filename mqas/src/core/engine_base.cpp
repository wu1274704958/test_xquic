#include <fstream>
#include<mqas/core/engine_base.h>
#include <sstream>
#include <stdexcept>

#include <mqas/io/udp.h>
#include <mqas/io/timer.h>

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

void mqas::core::engine_base::init(const char* conf_file)
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

mqas::core::engine_config toml::from<mqas::core::engine_config>::from_toml(const value& v)
{
	mqas::core::engine_config f;
	f.bind_ip =		find_or<std::string		>(v,	"bind_ip",	"0.0.0.0");
	f.port =		find_or<short			>(v,	"port",		8083);
	return f;
}
