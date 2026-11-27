#pragma once
#include "mqas/io/ip.h"

#include "mqas/macro.h"
#include "mqas/core/def.h"
#include <unordered_map>

namespace mqas::tools {

	template<typename T>
	class peer_context_mgr
	{
		public:
		core::peer_context<T>& get_or_create(const sockaddr* addr,T* t);
		protected:
		struct AddressHash {
            size_t operator()(const io::SocketAddress& address) const {
                return std::hash<sockaddr>{}(address.native());
            }
        };
        struct AddressEqual {
            bool operator()(const io::SocketAddress& a, const io::SocketAddress& b) const {
                return a.native() == b.native();
            }
        };
        std::unordered_map<io::SocketAddress, core::peer_context<T>, AddressHash, AddressEqual> map;
	};

}

#include "peer_context_mgr.impl.hpp"