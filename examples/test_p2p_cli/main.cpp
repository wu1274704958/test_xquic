#include <mqas/context.h>
#include <mqas/io/context.h>
#include "mqas/core/engine_base.h"
#include <iostream>
#include <mqas/core/engine.h>
#include <mqas/core/connect.h>
#include <mqas/core/pb_stream.h>
#include <mqas/tools/stream/p2p_lobby_client.h>
using namespace mqas;
MQAS_SHARE_EASYLOGGINGPP

using StreamType = core::StreamVariant<
    core::StreamVariantPair<1, tools::p2p::P2PLobbyClientStream>>;

int main(int argc,const char** argv)
{
	Context<core::InitFlags::GLOBAL_CLIENT> context;
	io::Context io_cxt;
	core::engine_base<core::engine<core::Connect<StreamType>>> e(io_cxt);
	try{
		e.init("conf.txt",core::EngineFlags::None);
		e.start_recv();
		e.process_conns();
        sockaddr addr{};
        io::Ip::str2addr_ipv4("127.0.0.1",8084,addr);
        auto c = e.get_engine()->connect(addr,N_LSQVER);
        auto conn = c.lock();
        auto t = io_cxt.make_handle<io::Timer>();
        std::weak_ptr<StreamType> stream_out;
        conn->make_stream([&io_cxt,&stream_out](std::weak_ptr<StreamType> stream){
            stream_out = stream;
            auto s = stream.lock();
			mqas::tools::proto::p2p::ReqRegistePeer msg;
			s->req_change<tools::p2p::P2PLobbyClientStream, mqas::tools::p2p::ReqRegistePeerPair>(msg);
        });
	}catch (std::exception& e)
	{
		std::cerr << e.what() << std::endl;
	}
	io_cxt.run_until(IsRunning());
	return 0;
}
