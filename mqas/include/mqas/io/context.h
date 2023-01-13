#pragma once
#include <uv.h>
#include <memory>
#include <mqas/macro.h>
#include <vector>
#include <atomic>
namespace mqas::io
{
	class Idle;
	class MQAS_EXTERN Context
	{
		enum class RunMode{
			DEFAULT = 0,
			ONCE,
			NOWAIT
		};
	public:
		Context();
		~Context();
		Context(const Context&)=delete;
		Context(Context&&) = delete;
		Context& operator=(const Context&) = delete;
		Context& operator=(Context&&) = delete;
		void close_loop();
		void run(RunMode mode = RunMode::DEFAULT) const;
		void run_until(std::atomic_bool&) ;
		void stop() const;
		[[nodiscard]] std::shared_ptr<uv_loop_t> get_loop() const;
		template<typename H>
		requires requires(H h)
		{
			new H();
			h.init(std::declval<const Context&>());
		}
		H* make_handle()
		{
			H h;
			h.init(*this);
			if constexpr (std::is_same_v<H,Idle>)
			{
				IdleArr.push_back(std::move(h));
				return &IdleArr.back();
			}
			return nullptr;
		}
	protected:
		std::shared_ptr<uv_loop_t> loop;
		std::vector<Idle> IdleArr;
	};


}
