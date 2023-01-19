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
	conf_ = std::make_unique<toml::basic_value<toml::discard_comments>>(toml::parse(conf_file));
	const auto& conf = *conf_;
	const auto ip = toml::find_or<std::string>(conf,"bind_ip","0.0.0.0");
	const auto port = toml::find_or<short>(conf,"port",8083);
	//init socket
	socket_ = cxt.make_handle<io::UdpSocket>();
	sockaddr addr{};
	io::UdpSocket::str2addr_ipv4(ip.c_str(),port, addr);
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
