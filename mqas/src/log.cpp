#include <mqas/log.h>
#include <easylogging++.h>

//namespace el {
//	namespace base {
//		 ELPP_EXPORT el::base::type::StoragePointer elStorage(new el::base::Storage(el::LogBuilderPtr(new el::base::DefaultLogBuilder())));
//	}
//	el::base::debug::CrashHandler elCrashHandler(ELPP_USE_DEF_CRASH_HANDLER);
//}
INITIALIZE_EASYLOGGINGPP

void mqas::log::init(const std::string& conf)
{
	if(!conf.empty())
	{
		el::Configurations c;
		c.setToDefault();
		c.parseFromText(conf);
		el::Loggers::reconfigureAllLoggers(c);
	}
}

void mqas::log::init(const char* log_name, const std::string& conf, const std::optional<std::string> base)
{
	el::Configurations c;
	if(base)
		c.setFromBase(el::Loggers::getLogger(base.value())->configurations());
	else
		c.setToDefault();
	c.parseFromText(conf);
	el::Loggers::reconfigureLogger(el::Loggers::getLogger(log_name),c);
}
