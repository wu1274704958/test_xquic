#include <mqas/context.h>
#include <mqas/io/context.h>
#include "mqas/io/timer.h"

using namespace mqas;

int main(int argc,const char** argv)
{
	Context context;
	io::Context io_cxt;

	int a = 0;
	auto timer = io_cxt.make_handle<io::Timer>();
	timer->start([&](io::Timer* t)
	{
		printf("timer %d\n",++a);
		if (a == 3) t->stop();
	},1000,1000);

	io_cxt.run_until(Context::IsRunning);
	return 0;
}