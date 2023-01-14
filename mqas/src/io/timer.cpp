#include <mqas/io/timer.h>
#include <mqas/io/exception.h>


void mqas::io::TimerOp::init(std::shared_ptr<uv_timer_t> h, std::shared_ptr<uv_loop_t> l)
{
	if(const int ret = uv_timer_init(l.get(),h.get()); ret != 0)
		throw Exception(ret);
}

void mqas::io::Timer::start(std::function<void(Timer*)> f, uint64_t timeout, uint64_t repeat)
{
	if(const int ret = uv_timer_start(handle_.get(),timer_cb_static,timeout,repeat); ret!=0)throw Exception(ret);
	cb_func_ = f;
}

void mqas::io::Timer::stop()
{
	cb_func_ = nullptr;
	if (const int ret = uv_timer_stop(handle_.get()); ret != 0)
		throw Exception(ret);
}

void mqas::io::Timer::set_repeat(uint64_t repeat) const
{
	uv_timer_set_repeat(handle_.get(),repeat);
}

uint64_t mqas::io::Timer::get_repeat() const
{
	return uv_timer_get_repeat(handle_.get());
}

uint64_t mqas::io::Timer::get_due_in() const
{
	return uv_timer_get_due_in(handle_.get());
}

void mqas::io::Timer::again() const
{
	if (const int ret = uv_timer_again(handle_.get()); ret != 0)
		throw Exception(ret);
}

void mqas::io::Timer::timer_cb_static(uv_timer_t* handle)
{
	Timer* ptr = static_cast<Timer*>(handle->data);
	if(ptr && ptr->cb_func_)
		ptr->cb_func_(ptr);
}

namespace mqas::io
{
	template class Handle<uv_timer_t,TimerOp>;
}


