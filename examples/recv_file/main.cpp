#include <mqas/context.h>
#include <mqas/io/context.h>
#include "mqas/core/engine_base.h"
#include <iostream>


using namespace mqas;

class Engine : public mqas::core::IEngine
{
public:
	io::UdpSocket* get_udp() const;
};

io::UdpSocket* Engine::get_udp() const
{
	const auto p = static_cast<core::engine_base<Engine>*>(engine_base_ptr_);
	return p->socket_;
}

int main(int argc,const char** argv)
{
	Context<core::InitFlags::GLOBAL_SERVER> context;
	io::Context io_cxt;
	core::engine_base<Engine> e(io_cxt);
	try{
		e.init("conf.txt",core::EngineFlags::Server);
		e.start_recv();
		e.process_conns();
	}catch (std::exception& e)
	{
		std::cerr << e.what() << std::endl;
	}
	io_cxt.run_until(is_running());
	return 0;
}
