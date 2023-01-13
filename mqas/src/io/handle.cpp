#include <mqas/io/handle.h>

namespace mqas::io
{
	HandleBase::HandleBase()
	{
		handle_ = std::make_shared<uv_handle_t>();
	}
	HandleBase::HandleBase(HandleBase&& oth) noexcept
	{
		handle_.swap(oth.handle_);
		if (handle_)
			handle_->data = this;
	}

	HandleBase& HandleBase::operator=(HandleBase&& oth) noexcept
	{
		handle_ = std::move(oth.handle_);
		if (handle_)
			handle_->data = this;
		return *this;
	}
}