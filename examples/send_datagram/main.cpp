#include <mqas/context.h>
#include <mqas/io/context.h>
#include <iostream>
#include <mqas/core/connect.h>
#include <format>


using namespace mqas;


class Stream : public core::IStream
{
};

int main(int argc,const char** argv)
{
	Context<core::InitFlags::GLOBAL_CLIENT> context;
	io::Context io_cxt;
	core::engine_base<core::engine<core::Connect<Stream>>> e(io_cxt);
	try{
		e.init("conf.txt",core::EngineFlags::None);
		e.start_recv();
		e.process_conns();
	}catch (std::exception& e)
	{
		std::cerr << e.what() << std::endl;
	}
	::sockaddr addr{};
	io::Ip::str2addr_ipv4("127.0.0.1",8084,addr);
	auto conn_w = e.get_engine()->connect(addr,::lsquic_version::N_LSQVER);
	auto timer = io_cxt.make_handle<io::Timer>();
	timer->start([conn_w](io::Timer* t) {
		static int send_times = 0;
		printf("try %d\n",send_times);
		auto c = conn_w.lock();
		if (!c || c->status() != LSQUIC_CONN_STATUS::LSCONN_ST_CONNECTED) {
			t->stop();
			return;
		}
		std::string msg = std::format("lalalala!!! {}\0",send_times);
		if (c->write_datagram(std::span(reinterpret_cast<uint8_t*>(msg.data()),msg.size())))
		{
			std::cout << "send success\n";
			send_times++;
			if(send_times >= 100)
				t->stop();
		}
	},1000,1000);
	io_cxt.run_until(IsRunning());
	return 0;
}
