#pragma once

#include <mqas/macro.h>
#include <exception>
namespace mqas::io
{
	class MQAS_EXTERN Exception : public std::exception
	{
	public:
		explicit Exception(int);
		[[nodiscard]] char const* what() const override;
		[[nodiscard]] char const* err_name() const;
		[[nodiscard]] int error_code() const;
	protected:
		int code_;
	};
}