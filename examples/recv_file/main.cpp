#include <mqas/context.h>
#include <mqas/io/context.h>
#include "mqas/core/engine_base.h"
#include <iostream>
#include <mqas/core/engine.h>


using namespace mqas;


class Conn : public core::IConnect
{

};

int main(int argc,const char** argv)
{
	Context<core::InitFlags::GLOBAL_SERVER> context;
	io::Context io_cxt;
	core::engine_base<core::engine<Conn>> e(io_cxt);
	try{
		e.init("conf.txt",core::EngineFlags::Server);
		e.start_recv();
		e.process_conns();
	}catch (std::exception& e)
	{
		std::cerr << e.what() << std::endl;
	}
	io_cxt.run_until(IsRunning());
	return 0;
}
