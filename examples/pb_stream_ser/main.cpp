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
    static constexpr size_t STREAM_TAG = 1;
    core::StreamVariantErrcode on_change_msg_s(const std::shared_ptr<proto::SayHelloMsg>& hello,
                                               std::vector<uint8_t> &ret_buf)
    {
        printf("on_change %s %d\n",hello->msg().c_str(),hello->num());
        hello->set_num(hello->num() * 2);
        core::ProtoBufMsg::write_msg<SayHelloMsgPair>(ret_buf,*hello);
        return core::StreamVariantErrcode::ok;
    }

    void on_read_msg_s(const std::shared_ptr<proto::SayHelloMsg>& m)
    {
        printf("on_read %s %d %zu\n",m->msg().c_str(),m->num(),(size_t)stream_);
        proto::SayHelloMsg helloMsg;
        helloMsg.set_num(m->num() + 1);
        helloMsg.set_msg("copy that");
        send <SayHelloMsgPair>(helloMsg);
        setIsWaitPeerChangeRet(true);
    }
    mqas::core::StreamVariantErrcode on_peer_quit_msg_s(const std::shared_ptr<proto::SayByeMsg>& m,
                                      std::vector<uint8_t>& buf)
    {
        printf("on_peer_quit %d %d\n",m->num(),m->num2());
        m->set_num2(m->num2() + 1);
        core::ProtoBufMsg::write_msg<SayByeMsgPair>(buf,*m);
        return mqas::core::StreamVariantErrcode::ok;
    }

};

int main(int argc,const char** argv)
{
	Context<core::InitFlags::GLOBAL_SERVER> context;
	io::Context io_cxt;
	core::engine_base<core::engine<core::Connect<core::StreamVariant<Stream>>>> e(io_cxt);
	try{
		e.init("conf.txt",core::EngineFlags::Server);
		e.start_recv();
		e.process_conns();
	}catch (std::exception& e)
	{
		std::cerr << e.what() << std::endl;
	}
	io_cxt.run_until(IsRunning());
	return 0;
}
