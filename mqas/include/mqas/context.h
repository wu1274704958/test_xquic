#pragma once
#include <memory>
#include <atomic>
#include <mqas/macro.h>

namespace mqas
{
	class MQAS_EXTERN Context
	{
		
	public:
		Context();
		~Context();
		Context(const Context&)=delete;
		Context(Context&&) = delete;
		Context& operator=(const Context&) = delete;
		Context& operator=(Context&&) = delete;
	protected:
		static void sig_handler(int);
	public:
		static std::atomic_bool IsRunning;
	};


}
