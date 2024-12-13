#pragma once
#include <memory>
#include <mqas/macro.h>
#include <uv.h>

#include <mqas/io/context.h>

namespace mqas::io
{
	template<typename H,typename HandleOp>
	requires requires(H h){
		H();
		HandleOp::init(std::declval<H*>(), std::declval<std::shared_ptr<uv_loop_t>>());
		std::is_same_v<decltype(h.data),void*> == true;
	}
	class MQAS_EXTERN Handle
	{
	public:
		void init(const Context& cxt)
		{
			handle_ = new H();
			HandleOp::init(handle_,cxt.get_loop());
			handle_->data = this;
		}
		
		template<typename ... Args>
		requires requires() {
			HandleOp::init(std::declval<std::shared_ptr<H>>(),
				std::declval<std::shared_ptr<uv_loop_t>>(),
				std::declval<Args>()...);
		}
		void init(const Context& cxt,Args&& ...args)
		{
			HandleOp::init(handle_, cxt.get_loop(),std::forward<Args>(args) ...);
			handle_->data = this;
		}
		Handle()
		{
			handle_ = nullptr;
		}
		~Handle()
		{
			deinit();
		}
		Handle(const Handle&)=delete;
		Handle(Handle&& oth) noexcept
		{
			handle_ = oth.handle_;
			data = oth.data;
			oth.data = nullptr;
			oth.handle_ = nullptr;
			if(handle_ != nullptr)
				handle_->data=this;
		}
		Handle& operator=(const Handle&) = delete;
		Handle& operator=(Handle&& oth) noexcept
		{
			deinit();
			handle_ = oth.handle_;
			data = oth.data;
			oth.data = nullptr;
			oth.handle_ = nullptr;
			if (handle_ != nullptr)
				handle_->data = this;
			return *this;
		}
		H* get_ptr()
		{
			return handle_;
		}
	protected:
		void deinit()
		{
			if (handle_ != nullptr && will_close_ == false)
			{
				will_close_ = true;
				::uv_close((::uv_handle_t*)handle_, on_close);
			}
		}
		static void on_close(uv_handle_t* handle)
		{
			delete handle;
		}
	public:
		void *data = nullptr;
	protected:
		H* handle_;
		bool will_close_:1 = false;
	};
}
