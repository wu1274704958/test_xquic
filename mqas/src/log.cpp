#include <mqas/log.h>
#include <easylogging++.h>

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
