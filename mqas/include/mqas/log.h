#pragma once
#include <easylogging++.h>
#include <optional>
#include <string>
#include <mqas/macro.h>

namespace mqas
{
	class MQAS_EXTERN log
	{
	public:
		static void init(const std::string&);
		static void init(const char* log_name, const std::string& conf,  const std::optional<std::string>& base);
	};
}