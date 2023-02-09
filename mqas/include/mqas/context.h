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
	MQAS_EXTERN std::atomic_bool& is_running();

	template<core::InitFlags F>
	class Context
	{
	public:
		Context()
		{
			if (is_running())
			{
				throw std::runtime_error("Context instance must only one!");
			}
			is_running() = true;
			::signal(SIGINT, sig_handler);
			if (0 != ::lsquic_global_init(static_cast<int>(F)))
			{
				throw std::runtime_error("Init lsquic failed!");
			}
		}
		static constexpr bool contain_server()
		{
			return (static_cast<uint32_t>(F) & static_cast<uint32_t>(core::InitFlags::GLOBAL_SERVER)) == static_cast<uint32_t>(core::InitFlags::GLOBAL_SERVER);
		}
		static constexpr bool contain_client()
		{
			return (static_cast<uint32_t>(F) & static_cast<uint32_t>(core::InitFlags::GLOBAL_CLIENT)) == static_cast<uint32_t>(core::InitFlags::GLOBAL_CLIENT);
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
				is_running() = false;
		}
	public:
	};


}
