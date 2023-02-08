#include <mqas/context.h>
#include <mqas/io/context.h>
#include "mqas/core/engine_base.h"
#include <iostream>

using namespace mqas;

int main(int argc,const char** argv)
{
	Context context;
	io::Context io_cxt;

	core::engine_base e(io_cxt);
	try{
		e.init("conf.txt");
	}catch (std::exception& e)
	{
		std::cerr << e.what() << std::endl;
	}
	io_cxt.run_until(Context::IsRunning);
	return 0;
}