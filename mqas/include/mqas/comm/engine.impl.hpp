#include "mqas/log.h"

namespace mqas::comm {

	template<class S, template < class > class C, template< class > class E>
		requires requires
	{
		requires std::is_default_constructible_v<E<C<S>>>;
		requires std::is_base_of_v<core::IEngine, E<C<S>>>;
		requires std::is_default_constructible_v<C<S>>;
		requires std::is_base_of_v<core::IConnect, C<S>>;
	}
	auto engine_util::launch_engine(io::Context& io_cxt,const char* conf_file, mqas::core::EngineFlags engine_flags,
		std::shared_ptr<io::UdpSocket> socket, std::function<void(std::shared_ptr<C<S>>)> on_connected,
		std::optional<sockaddr> addr) -> std::shared_ptr<mqas::core::engine_base<E<C<S>>>>
	{
		auto engine = std::make_shared<core::engine_base<E<C<S>>>>(io_cxt);
		try {
			if(socket)
				engine->init(conf_file, engine_flags, socket);
			else
				engine->init(conf_file, engine_flags);
			engine->start_recv();
			engine->process_conns();

			if (addr)
			{
				auto conn = engine->get_engine()->connect(*addr, N_LSQVER);
				if (on_connected)
				{
					auto c = conn.lock();
					on_connected(c);
				}
			}
			else {
				engine->get_engine()->on_new_connect_signal.connect(on_connected);
			}
		}
		catch (std::exception& e)
		{
			LOG(ERROR) << "Launch engine failed: " << e.what();
		}
		return engine;
	}
}