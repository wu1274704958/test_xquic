#pragma once
#include <type_traits>
#include "mqas/core/engine.h"
#include "mqas/core/connect.h"
#include <optional>
#include "mqas/core/sub_engine.h"

namespace mqas::comm {
	class engine_util {
		
	public:

		template<class S, 
			template< class > class C = core::Connect,
			template< class > class E = core::engine,
			template< class, class, class> class SE = core::sub_engine, class ED = core::engine_driver,class SC = io::UdpSocket>
			requires requires
		{
			requires std::is_default_constructible_v<E<C<S>>>;
			requires std::is_base_of_v<core::IEngine, E<C<S>>>;
			requires std::is_default_constructible_v<C<S>>;
			requires std::is_base_of_v<core::IConnect, C<S>>;
			requires core::IsVaildEngineDriver<ED>;
			requires core::IsVaildSocket<SC>;
		}
		static auto launch_sub_engine(io::Context& io_cxt, const char* conf_file, mqas::core::EngineFlags engine_flags,
			std::shared_ptr<SC> socket = nullptr, std::function<void(std::shared_ptr<C<S>>)> on_connected = {},
			const sockaddr* addr = nullptr, std::function<void(const std::exception&)> on_exception = {},
			std::function<void(SE<E<C<S>>,ED,SC>&)> on_engine_init = {})
			-> std::shared_ptr<SE<E<C<S>>,ED,SC>>;
	};
}

#include "engine.impl.hpp"