#pragma once

#include <unordered_map>
#include <any>
#include <typeindex>
#include <memory>
#include <optional>

namespace mqas::comm {
	class locator {

		public:
			static std::shared_ptr<locator> inst();
			template<typename T>
			std::optional<std::reference_wrapper<T>> get()
			{
				T* p = try_get_from_pool<T>(map);
				if (p == nullptr)
					return {};
				else
					return *p;
			}
			template<typename T,typename ... Args>
			bool deposit(Args&& ...args)
			{
				return deposit<T>(map,false,std::forward<Args>(args)...);
			}
			template<typename T, typename ... Args>
			bool deposit_force(Args&& ...args)
			{
				return deposit<T>(map, true, std::forward<Args>(args)...);
			}
			template<typename T>
			void remove()
			{
				remove_from_pool<T>(map);
			}
			void clear();

			template<typename T, typename O>
			std::optional<std::reference_wrapper<T>> get(const O& obj)
			{
				auto pool = get_pool(obj);
				if(!pool)
					return {};
				T* p = try_get_from_pool<T>(*pool);
				if (p == nullptr)
					return {};
				else
					return *p;
			}
			template<typename T,typename O, typename ... Args>
			bool deposit_cxt(const O& obj,Args&& ...args)
			{
				auto pool = get_pool(obj,true);
				if (!pool)
					return false;
				return deposit<T>(*pool, false, std::forward<Args>(args)...);
			}
			template<typename T,typename O, typename ... Args>
			bool deposit_cxt_force(const O& obj,Args&& ...args)
			{
				auto pool = get_pool(obj,true);
				if (!pool)
					return false;
				return deposit<T>(*pool, true, std::forward<Args>(args)...);
			}
			template<typename T,typename O>
			void remove(const O& obj)
			{
				auto pool = get_pool(obj);
				if (!pool)
					return;
				remove_from_pool<T>(*pool);
				if(pool->empty())
					remove_pool(obj);
			}
			template<typename O>
			void clear_by_context(const O& obj)
			{
				auto pool = get_pool(obj);
				if (!pool)
					return;
				pool->clear();
				remove_pool(obj);
			}

		protected:
			template<typename O>
			std::unordered_map<std::type_index, std::any>* get_pool(const O& obj,bool create = false)
			{
				auto key = reinterpret_cast<void*>(const_cast<O*>(&obj));
				auto val = map_for_obj.find(key);
				if (val == map_for_obj.end())
				{
					if (create)
					{
						map_for_obj.insert(std::make_pair(key,std::unordered_map<std::type_index,std::any>()));
						return &map_for_obj[key];
					}else
						return nullptr;
				}
				return &val->second;
			}
			template<typename O>
			void remove_pool(const O& obj)
			{
				auto key = reinterpret_cast<void*>(const_cast<O*>(&obj));
				map_for_obj.erase(key);
			}

			template<typename T>
			T* try_get_from_pool(std::unordered_map<std::type_index, std::any>& pool)
			{
				const auto val = pool.find(std::type_index(typeid(T)));
				if (val != pool.end())
				{
					try{
						return std::any_cast<T>(&val->second);
					}
					catch (...)
					{
						return nullptr;
					}
				}
				return nullptr;
			}
			template<typename T>
			void remove_from_pool(std::unordered_map<std::type_index, std::any>& pool)
			{
				const auto key = std::type_index(typeid(T));
				pool.erase(key);
			}
			template<typename T, typename ... Args>
			bool deposit(std::unordered_map<std::type_index, std::any>& pool,bool force,Args&& ...args)
			{
				const auto key = std::type_index(typeid(T));
				const auto val = pool.find(key);
				if (!force && val != pool.end())
						return false;	
				pool[key] = std::any(std::in_place_type<T>, std::forward<Args>(args)...);
				return true;
			}
		protected:
		static std::shared_ptr<locator> instance;
		
		protected:
		std::unordered_map<std::type_index,std::any> map;
		std::unordered_map<void*, std::unordered_map<std::type_index, std::any>> map_for_obj;
	};
	
}