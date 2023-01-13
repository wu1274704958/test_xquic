#include <mqas/io/exception.h>
#include <uv.h>
namespace mqas::io
{
	Exception::Exception(int code)
	{
		this->code_ = code;
	}

	char const* Exception::what() const
	{
		return uv_strerror(code_);
	}

	char const* Exception::err_name() const
	{
		return uv_err_name(code_);
	}

	int Exception::error_code() const
	{
		return code_;
	}
}
