#pragma once

#include <mqas/macro.h>
#include <uv.h>
#include <memory>
#include <functional>
#include <mqas/io/handle.h>

namespace mqas::io
{
	class MQAS_EXTERN TimerOp
	{
	public:
		static void init(std::shared_ptr<uv_timer_t>, std::shared_ptr<uv_loop_t>);
	};
	class MQAS_EXTERN Timer : public Handle<uv_timer_t, TimerOp>
	{
	public:
		Timer() = default;
		Timer(Timer&&) = default;
		Timer(const Timer&) = delete;
		Timer& operator=(Timer&&) = default;
		Timer& operator=(const Timer&) = delete;
		void start(std::function<void(Timer*)>,uint64_t timeout,uint64_t repeat);
		void stop();
		void set_repeat(uint64_t repeat) const;
		[[nodiscard]] uint64_t get_repeat() const;
		[[nodiscard]] uint64_t get_due_in() const;
		void again() const;
		static void timer_cb_static(uv_timer_t* handle);
	protected:
		std::function<void(Timer*)> cb_func_;
	};
}