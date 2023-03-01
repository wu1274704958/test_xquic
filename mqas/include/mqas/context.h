#pragma once
#include <memory>
#include <atomic>
#include <mqas/macro.h>
#include <mqas/core/def.h>
#include <stdexcept>
#include <signal.h>
#include <lsquic.h>

namespace mqas
{
	MQAS_EXTERN std::atomic_bool& IsRunning();

	template<core::InitFlags F>
	class Context
	{
	public:
		Context()
		{
			if (IsRunning())
			{
				throw std::runtime_error("Context instance must only one!");
			}
            IsRunning() = true;
			::signal(SIGINT, sig_handler);
			if (0 != ::lsquic_global_init(static_cast<int>(F)))
			{
				throw std::runtime_error("Init lsquic failed!");
			}
		}
		~Context()
		{
			::lsquic_global_cleanup();
		}
		Context(const Context&)=delete;
		Context(Context&&) = delete;
		Context& operator=(const Context&) = delete;
		Context& operator=(Context&&) = delete;
	protected:
		static void sig_handler(int sig)
		{
			if (sig == SIGINT)
                IsRunning() = false;
		}
	public:
	};


}
