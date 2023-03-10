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
        static el::base::type::StoragePointer elStorage;
		static void init(const std::string&);
		static void init(const char* log_name, const std::string& conf,  const std::optional<std::string>& base);
	};
}

#if WIN32 && MQAS_SHARED
#define MQAS_SHARE_EASYLOGGINGPP SHARE_EASYLOGGINGPP(mqas::log::elStorage)
#else
#define MQAS_SHARE_EASYLOGGINGPP
#endif