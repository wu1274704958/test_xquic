#pragma once
#include <uv.h>
#include <memory>
#include <mqas/macro.h>
#include <unordered_map>
#include <atomic>
#include <cassert>
#include <typeindex>

namespace mqas::io
{
	class Idle;
	class Timer;
	class UdpSocket;
	class MQAS_EXTERN Context
	{
	public:
		enum class RunMode{
			DEFAULT = 0,
			ONCE,
			NOWAIT
		};
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
			auto t = new H();
			t->init(*this, std::forward<Args>(args) ...);
			const auto key = (size_t)t->get_ptr();
			handle_map.insert({ key, std::make_pair<void*,std::type_index>((void*)t,std::type_index(typeid(H)))});
			return t;
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
		template <typename H>
		void del_handle(H* ptr)
		{
			const auto key = (size_t)ptr->get_ptr();
			if (handle_map.find(key) != handle_map.end())
			{
				handle_map.erase(key);
				delete ptr;
			}
		}
	protected:
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-value"
		template<typename ...HS>
		void delete_hold_handle_ptr()
		{
			for (const auto& it : handle_map)
			{
				((it.second.second == std::type_index(typeid(HS)) && (delete_hold_handle_ptr_by_type<HS>(reinterpret_cast<HS*>(it.second.first),it.first),false)),...);
			}
			handle_map.clear();
		}
#pragma GCC diagnostic pop
		template<typename H>
		void delete_hold_handle_ptr_by_type(H* ptr,size_t key)
		{
			delete ptr;
		}
	protected:
		std::shared_ptr<uv_loop_t> loop;
		std::unordered_map<size_t,std::pair<void*,std::type_index>> handle_map;
	};
	
}
