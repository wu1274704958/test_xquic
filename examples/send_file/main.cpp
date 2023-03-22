#include <mqas/context.h>
#include <mqas/io/context.h>
#include "mqas/core/engine_base.h"
#include <iostream>
#include <mqas/core/engine.h>
#include <mqas/core/connect.h>
#include <mqas/core/pb_stream.h>
#include <mqas/tools/stream/send_file.h>
using namespace mqas;
MQAS_SHARE_EASYLOGGINGPP
using Stream = core::StreamVariant<
        core::StreamVariantPair<1,tools::SendFileStream>
>;
int main(int argc,const char** argv)
{
    if(argc < 2) return -1;
	Context<core::InitFlags::GLOBAL_CLIENT> context;
	io::Context io_cxt;
	core::engine_base<core::engine<core::Connect<Stream>>> e(io_cxt);
	try{
		e.init("conf.txt",core::EngineFlags::None);
		e.start_recv();
		e.process_conns();
        sockaddr addr{};
        io::Ip::str2addr_ipv4("127.0.0.1",8084,addr);
        auto c = e.get_engine()->connect(addr,N_LSQVER);
        auto conn = c.lock();
        auto t = io_cxt.make_handle<io::Timer>();
        conn->make_stream([argv](std::weak_ptr<Stream> stream){
            auto s = stream.lock();
            if(s)
            {
                tools::proto::ReqSendFile msg;
                msg.set_name(argv[1]);
                s->req_change<tools::SendFileStream,tools::ReqSendFileMsgPair>(msg);
            }
        });
	}catch (std::exception& e)
	{
		std::cerr << e.what() << std::endl;
	}
	io_cxt.run_until(IsRunning());
	return 0;
}
