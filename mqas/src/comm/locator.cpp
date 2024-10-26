#include "mqas/comm/locator.h"

namespace mqas::comm {

std::shared_ptr<locator> locator::instance = std::make_shared<locator>();

std::shared_ptr<locator> locator::inst()
{
	return instance;
}

void locator::clear()
{
	map.clear();
}

}
