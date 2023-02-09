#pragma once
#include <optional>
#include <string>
#include <mqas/macro.h>

namespace mqas
{
	class MQAS_EXTERN log
	{
	public:
		static void init(const std::string&);
	};
}