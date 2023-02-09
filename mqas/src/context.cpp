#include <csignal>
#include <lsquic.h>
#include <mqas/context.h>
#include <stdexcept>

namespace mqas
{
	std::atomic_bool& is_running()
	{
		static std::atomic_bool is_running;
		return is_running;
	}
}
