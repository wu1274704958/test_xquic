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
		HandleOp::init(std::declval<std::shared_ptr<H>>(), std::declval<std::shared_ptr<uv_loop_t>>());
		std::is_same_v<decltype(h.data),void*> == true;
	}
	class MQAS_EXTERN Handle
	{
	public:
		void init(const Context& cxt)
		{
			HandleOp::init(handle_,cxt.get_loop());
			handle_->data = this;
		}
		Handle()
		{
			handle_ = std::make_shared<H>();
		}
		Handle(const Handle&)=delete;
		Handle(Handle&& oth) noexcept
		{
			handle_.swap(oth.handle_);
			if(handle_)
				handle_->data=this;
		}
		Handle& operator=(const Handle&) = delete;
		Handle& operator=(Handle&& oth) noexcept
		{
			handle_ = std::move(oth.handle_);
			if (handle_)
				handle_->data = this;
			return *this;
		}
		H* get_ptr()
		{
			return handle_.get();
		}
	protected:
		std::shared_ptr<H> handle_;
	};
}
