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
    auto file_path = argv[1];
    auto buf_size = 2048;
    bool overlay = false;
    if(argc >= 3)
        buf_size = atoi(argv[2]);
    if(argc >= 4)
        overlay = argv[3][0] == '1';
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
        conn->make_stream([file_path,buf_size,overlay](std::weak_ptr<Stream> stream){
            auto s = stream.lock();
            if(s)
            {
                std::cout << "connect stream \n";
                tools::proto::ReqSendFile msg;
                msg.set_name(file_path);
                msg.set_buf_size(buf_size);
                msg.set_overlay(overlay);
                printf("overlay = %d\n",msg.overlay());
                s->req_change<tools::SendFileStream,tools::ReqSendFileMsgPair>(msg);
                auto real_stream = s->get_holds_stream<tools::SendFileStream>();
                if(!real_stream)
                {
                    std::cout << "file not found or can not read" << std::endl;
                }
                real_stream->add_on_success_cb([](tools::SendFileStream* s){
                    printf("send success\n");
                    s->close();
                });
                real_stream->add_on_read_failed_cb([](tools::SendFileStream* s,ssize_t code){
                    std::cout <<  "read failed " << code << std::endl;
                    s->close();
                });
                real_stream->add_on_change_ret_err_cb([](tools::SendFileStream* s,const tools::proto::ReqSendFileRet& ret){
                    std::cout <<  "peer ret code " << ret.code() << " err = " << ret.error_code() << std::endl;
                    s->close();
                });
            }
        });
	}catch (std::exception& e)
	{
		std::cerr << e.what() << std::endl;
	}
	io_cxt.run_until(IsRunning());
	return 0;
}
