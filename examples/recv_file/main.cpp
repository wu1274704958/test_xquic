#include <mqas/context.h>
#include <mqas/io/context.h>
#include <mqas/io/idle.h>

using namespace mqas;

int main(int argc,const char** argv)
{
	Context context;
	io::Context io_cxt;

	io_cxt.run_until(Context::IsRunning);
	return 0;
}