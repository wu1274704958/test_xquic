#include <csignal>
#include <lsquic.h>
#include <mqas/context.h>
#include <stdexcept>

namespace mqas
{
	std::atomic_bool Context::IsRunning = false;
	Context::Context()
	{
		if(Context::IsRunning)
		{
			throw std::runtime_error("Context instance must only one!");
			return;
		}
		Context::IsRunning = true;
		::signal(SIGINT, sig_handler);
		if (0 != ::lsquic_global_init(LSQUIC_GLOBAL_SERVER))
		{
			throw std::runtime_error("Init lsquic failed!");
			return;
		}
	}

	Context::~Context()
	{
		::lsquic_global_cleanup();
	}

	void Context::sig_handler(int sig)
	{
		if (sig == SIGINT)
		{
			Context::IsRunning = false;
		}
	}
}
