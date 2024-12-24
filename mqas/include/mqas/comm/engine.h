#pragma once
#include <type_traits>
#include "mqas/core/engine.h"
#include "mqas/core/connect.h"
#include <optional>

namespace mqas::comm {
	class engine_util {
		
	public:
		template<class S, template < class > class C = core::Connect,template< class > class E = core::engine>
			requires requires
		{
			requires std::is_default_constructible_v<E<C<S>>>;
			requires std::is_base_of_v<core::IEngine,E<C<S>>>;
			requires std::is_default_constructible_v<C<S>>;
			requires std::is_base_of_v<core::IConnect,C<S>>;
		}
		static auto launch_engine(io::Context& io_cxt,const char* conf_file, mqas::core::EngineFlags engine_flags,
			std::shared_ptr<io::UdpSocket> socket = nullptr, std::function<void(std::shared_ptr<C<S>>)> on_connected = {},
			std::optional<sockaddr> addr = {}) -> std::shared_ptr<mqas::core::engine_base<E<C<S>>>>;
	};
}

#include "engine.impl.hpp"