#pragma once
#include <mqas/macro.h>
#include <uv.h>
#include <memory>
#include <functional>
#include <mqas/io/handle.h>

namespace mqas::io
{
	class IdleOp
	{
	public:
		static void init(std::shared_ptr<uv_idle_t>, std::shared_ptr<uv_loop_t>);
	};
	class MQAS_EXTERN Idle : public Handle<uv_idle_t,IdleOp>
	{
	public:
		Idle()=default;
		Idle(Idle&&) = default;
		Idle(const Idle&) = delete;
		Idle& operator=(Idle&&) = default;
		Idle& operator=(const Idle&) = delete;
		void start(std::function<void(Idle*)>);
		void stop();
		static void idle_cb_static(uv_idle_t* handle);
	protected:
		void idle_cb();
	protected:
		std::function<void(Idle*)> idle_func;
	};
}
