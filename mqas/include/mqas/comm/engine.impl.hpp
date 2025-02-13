#include "mqas/log.h"

namespace mqas::comm {

	template<class S,
		template< class > class C,
		template< class > class E,
		template< class, class, class> class SE , class ED, class SC>
		requires requires
	{
		requires std::is_default_constructible_v<E<C<S>>>;
		requires std::is_base_of_v<core::IEngine, E<C<S>>>;
		requires std::is_default_constructible_v<C<S>>;
		requires std::is_base_of_v<core::IConnect, C<S>>;
		requires core::IsVaildEngineDriver<ED>;
		requires core::IsVaildSocket<SC>;
	}
	auto engine_util::launch_sub_engine(io::Context& io_cxt, const char* conf_file, mqas::core::EngineFlags engine_flags,
		std::shared_ptr<SC> socket, std::function<void(std::shared_ptr<C<S>>)> on_connected,
		const sockaddr* addr, std::function<void(const std::exception&)> on_exception,
		std::function<void(SE<E<C<S>>,ED,SC>&)> on_engine_init)
		-> std::shared_ptr<SE<E<C<S>>, ED,SC>>
	{
		auto engine = std::make_shared<SE<E<C<S>>, ED, SC>>(io_cxt);
		try {
			if (socket)
				engine->init(conf_file, engine_flags, socket);
			else
				engine->init(conf_file, engine_flags);
			if (on_engine_init)
				on_engine_init(*engine);
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
				if(on_connected)
					engine->get_engine()->on_new_connect_signal.connect(on_connected);
			}
		}
		catch (std::exception& e)
		{
			LOG(ERROR) << "Launch engine failed: " << e.what();
			if (on_exception)
				on_exception(e);
		}
		return engine;
	}
}