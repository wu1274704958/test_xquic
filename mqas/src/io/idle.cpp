#include <mqas/io/idle.h>
#include <uv.h>

#include "mqas/io/exception.h" 

namespace mqas::io
{
	void IdleOp::init(std::shared_ptr<uv_idle_t> h, std::shared_ptr<uv_loop_t> loop)
	{
		if(const int ret = uv_idle_init(loop.get(), h.get())) throw Exception(ret);
	}

	void Idle::start(std::function<void(Idle*)> f)
	{
		idle_func = f;
		if(const int ret = uv_idle_start(handle_.get(),idle_cb_static)) throw Exception(ret);
	}

	void Idle::stop()
	{
		idle_func = nullptr;
		if (const int ret = uv_idle_stop(handle_.get())) throw Exception(ret);
	}

	void Idle::idle_cb_static(uv_idle_t* handle)
	{
		auto ptr = static_cast<Idle*>(handle->data);
		if(ptr) ptr->idle_cb();
	}

	void Idle::idle_cb()
	{
		if(idle_func)
			idle_func(this);
	}
}