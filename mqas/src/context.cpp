#include <mqas/context.h>
namespace mqas
{
	std::atomic_bool& is_running()
	{
		static std::atomic_bool is_running;
		return is_running;
	}
}
