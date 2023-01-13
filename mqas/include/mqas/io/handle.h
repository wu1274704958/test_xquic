#pragma once
#include <memory>
#include <mqas/macro.h>
#include <uv.h>
#include <mqas/io/context.h>

namespace mqas::io
{
	class Context;
	class MQAS_EXTERN HandleBase
	{
	public:
		HandleBase();
		template<typename H>
		H* get_ptr()
		{
			return reinterpret_cast<H*>(handle_.get());
		}
		HandleBase(const HandleBase&) = delete;
		HandleBase(HandleBase&& oth) noexcept;
		HandleBase& operator=(const HandleBase&) = delete;
		HandleBase& operator=(HandleBase&& oth) noexcept;
	protected:
		std::shared_ptr<uv_handle_t> handle_;
	};

	template<typename H, typename HandleOp>
	requires requires(H h) {
		H();
		HandleOp::init(std::declval<H*>(), std::declval<std::shared_ptr<uv_loop_t>>());
		std::is_same_v<decltype(h.data), void*> == true;
	}
	class MQAS_EXTERN Handle:public HandleBase
	{
	public:
		void init(const Context& cxt)
		{
			HandleOp::init(get_ptr<H>(),cxt.get_loop());
			handle_->data = this;
		}
	protected:
	};
}
