#pragma once

#include "mqas/macro.h"
#include "mqas/core/def.h"
#include <unordered_map>

namespace mqas::tools {

	template<typename T>
	class MQAS_EXTERN peer_context_mgr
	{
		public:
		core::peer_context<T>& get_or_create(const sockaddr* addr,T* t);
		protected:
		std::unordered_map<sockaddr,core::peer_context<T>> map;
	};

}

#include "peer_context_mgr.impl.hpp"