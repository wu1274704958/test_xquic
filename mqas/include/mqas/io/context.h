#pragma once
#include <uv.h>
#include <memory>
#include <mqas/macro.h>
#include <list>
#include <atomic>
#include <cassert>

namespace mqas::io
{
	class Idle;
	class Timer;
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
			std::list<H>* arr_ptr = get_handle_arr<H>();
			arr_ptr->emplace_back();
			H* h = &arr_ptr->back();
			h->init(*this);
			return h;
		}
		template<typename H>
		std::list<H>* get_handle_arr()
		{
			std::list<H>* arr_ptr = nullptr;
			if constexpr (std::is_same_v<H, Idle>)
			{
				arr_ptr = &idle_arr_;
			}
			if constexpr (std::is_same_v<H, Timer>)
			{
				arr_ptr = &timer_arr_;
			}
			assert(arr_ptr != nullptr);
			return arr_ptr;
		}
		template <typename H>
		void del_handle(H* ptr)
		{
			auto arr = get_handle_arr<H>();
			ptr->data = arr;
			uv_close(reinterpret_cast<uv_handle_t*>(ptr->get_ptr()), [](uv_handle_t* h)
			{
				auto p = static_cast<H*>(h->data);
				auto arr = static_cast<std::list<H>*>(p->data);
				for (auto it = arr->begin(); it != arr->end(); )
				{
					if (it->get_ptr() == p->get_ptr())
						it = arr->erase(it);
					else
						++it;
				}
			});
		}
	protected:
		std::shared_ptr<uv_loop_t> loop;
		std::list<Idle> idle_arr_;
		std::list<Timer> timer_arr_;
	};
	
}
