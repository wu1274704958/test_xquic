#include <mqas/io/context.h>
#include <mqas/io/idle.h>
#include <memory>
#include <mqas/io/timer.h>
#include <mqas/io/udp.h>
namespace mqas::io
{
	Context::Context()
	{
		loop = std::make_shared<uv_loop_t>();
		::uv_loop_init(loop.get());
	}

	Context::~Context()
	{
		close_loop();
	}

	void Context::close_loop()
	{
		if (!loop)return;
		if(::uv_loop_close(loop.get()) == 0)
			loop.reset();
	}

	void Context::run(RunMode mode) const
	{
		if(loop)
			::uv_run(loop.get(),static_cast<uv_run_mode>(mode));
	}

	void Context::run_until(std::atomic_bool& v) 
	{
		if (loop)
		{
			Context& cxt = *this;
			auto idle = make_handle<io::Idle>();
			idle->start([&](io::Idle* self)
			{
				if (!v)
					cxt.stop();
			});
			::uv_run(loop.get(), uv_run_mode::UV_RUN_DEFAULT);
		}
	}

	void Context::stop() const
	{
		if (loop)
			::uv_stop(loop.get());
	}

	std::shared_ptr<uv_loop_t> Context::get_loop() const 
	{
		return loop;
	}
}
