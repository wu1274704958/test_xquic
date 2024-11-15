#pragma once
#include <uv.h>
#include <memory>
#include <mqas/macro.h>
#include <unordered_map>
#include <atomic>
#include <cassert>

namespace mqas::io
{
	class Idle;
	class Timer;
	class UdpSocket;
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
		template<typename H,typename ...Args>
		requires requires(H h)
		{
			new H();
			h.init(std::declval<const Context&>(),std::declval<Args>()...);
		}
		H* make_handle(Args&& ...args)
		{
			std::unordered_map<void*, H>* arr_ptr = get_handle_arr<H>();
			auto t = H();
			void* key = t.get_ptr();
			t.init(*this, std::forward<Args>(args)...);
			arr_ptr->insert(std::make_pair(key,std::move(t)));
			return &(*arr_ptr)[key];
		}

		template<typename H, typename ...Args>
			requires requires(H h)
		{
			new H();
			h.init(std::declval<const Context&>(), std::declval<Args>()...);
		}
		std::shared_ptr<H> make_shared(Args&& ...args)
		{
			auto h = std::make_shared<H>();
			h->init(*this, std::forward<Args>(args)...);
			return h;
		}
		template<typename H>
		std::unordered_map<void*,H>* get_handle_arr()
		{
			std::unordered_map<void*, H>* arr_ptr = nullptr;
			if constexpr (std::is_same_v<H, Idle>)
			{
				arr_ptr = &idle_arr_;
			}
			if constexpr (std::is_same_v<H, Timer>)
			{
				arr_ptr = &timer_arr_;
			}
			if constexpr (std::is_same_v<H, UdpSocket>)
			{
				arr_ptr = &udp_arr_;
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
				auto arr = static_cast<std::unordered_map<void*, H>*>(p->data);
				arr->erase(p->get_ptr());
			});
		}
	protected:
		std::shared_ptr<uv_loop_t> loop;
		std::unordered_map<void*,Idle> idle_arr_;
		std::unordered_map<void*,Timer> timer_arr_;
		std::unordered_map<void*,UdpSocket> udp_arr_;
	};
	
}
