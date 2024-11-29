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

std::unordered_map<std::type_index, std::any>* locator::get_pool(void* key, bool create)
{
	auto val = map_for_obj.find(key);
	if (val == map_for_obj.end())
	{
		if (create)
		{
			map_for_obj.insert(std::make_pair(key, std::unordered_map<std::type_index, std::any>()));
			return &map_for_obj[key];
		}
		else
			return nullptr;
	}
	return &val->second;
}

void locator::remove_pool(void* key)
{
	map_for_obj.erase(key);
}

void locator::clear_by_context(size_t key)
{
	auto pool = get_pool(get_key(key));
	if (!pool)
		return;
	pool->clear();
	remove_pool(get_key(key));
}

}
