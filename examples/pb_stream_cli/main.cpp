#include <mqas/context.h>
#include <mqas/io/context.h>
#include "mqas/core/engine_base.h"
#include <iostream>
#include <mqas/core/engine.h>
#include <mqas/core/connect.h>
#include <mqas/core/pb_stream.h>
#include "say_hello.pb.h"
using namespace mqas;
MQAS_SHARE_EASYLOGGINGPP
using SayHelloMsgPair = core::PBMsgPair<1,proto::SayHelloMsg>;
using SayByeMsgPair = core::PBMsgPair<2,proto::SayByeMsg>;
class Stream:public core::ProtoBufStream<Stream,SayHelloMsgPair,SayByeMsgPair>
{
public:
    core::StreamVariantErrcode on_change_msg_s(const std::shared_ptr<proto::SayHelloMsg>& hello,
                                               std::vector<uint8_t> &ret_buf)
    {
        printf("on_change %s %d\n",hello->msg().c_str(),hello->num());
        hello->set_num(hello->num() * 2);
        core::ProtoBufMsg::write_msg<SayHelloMsgPair>(ret_buf,*hello);
        return core::StreamVariantErrcode::ok;
    }
    void on_peer_change_ret_msg_s(mqas::core::StreamVariantErrcode code,const std::shared_ptr<proto::SayHelloMsg>& m)
    {
        printf("on_peer_change_ret %s %d\n",m->msg().c_str(),m->num());
        proto::SayHelloMsg helloMsg;
        helloMsg.set_num(m->num() + 1);
        helloMsg.set_msg("hello");
        send <SayHelloMsgPair>(helloMsg);
    }
    void on_read_msg_s(const std::shared_ptr<proto::SayHelloMsg>& m)
    {
        printf("on_read %s %d\n",m->msg().c_str(),m->num());
        proto::SayByeMsg bye;
        bye.set_num(1);
        bye.set_num2(2);
        send_req_quit<SayByeMsgPair>(STREAM_TAG,bye);
    }
    void on_peer_quit_ret_msg_s(core::StreamVariantErrcode e,const std::shared_ptr<proto::SayByeMsg>& bye)
    {
        printf("on_peer_quit_ret bye %d %d\n",bye->num(),bye->num2());
    }

};

int main(int argc,const char** argv)
{
	Context<core::InitFlags::GLOBAL_CLIENT> context;
	io::Context io_cxt;
	core::engine_base<core::engine<core::Connect<core::StreamVariant<core::StreamVariantPair<1,Stream>>>>> e(io_cxt);
	try{
		e.init("conf.txt",core::EngineFlags::None);
		e.start_recv();
		e.process_conns();
        sockaddr addr{};
        io::Ip::str2addr_ipv4("127.0.0.1",8084,addr);
        auto c = e.get_engine()->connect(addr,N_LSQVER);
        auto conn = c.lock();
        auto t = io_cxt.make_handle<io::Timer>();
        std::weak_ptr<core::StreamVariant<core::StreamVariantPair<1,Stream>>> stream_out;
        conn->make_stream([&io_cxt,&stream_out](std::weak_ptr<core::StreamVariant<core::StreamVariantPair<1,Stream>>> stream){
            stream_out = stream;
        });
        t->start([&stream_out](io::Timer* t){
            auto s = stream_out.lock();
            if(s && !s->has_holds_stream())
            {
                proto::SayHelloMsg msg;
                msg.set_msg("req by hello");
                msg.set_num(1);
                s->req_change<Stream,SayHelloMsgPair>(msg);
                printf("no hold stream req chenge -----------------------------------------\n");
            }
        },1000,1000);
	}catch (std::exception& e)
	{
		std::cerr << e.what() << std::endl;
	}
	io_cxt.run_until(IsRunning());
	return 0;
}
