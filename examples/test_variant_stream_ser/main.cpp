#include <mqas/context.h>
#include <mqas/io/context.h>
#include "mqas/core/engine_base.h"
#include <iostream>
#include <mqas/core/engine.h>
#include <mqas/core/connect.h>
#include <mqas/core/pb_stream.h>
#include "say_hello.pb.h"
#include <mqas/comm/locator.h>

using namespace mqas;
MQAS_SHARE_EASYLOGGINGPP
using SayHelloMsgPair = core::PBMsgPair<1, proto::SayHelloMsg>;
using SayByeMsgPair = core::PBMsgPair<2, proto::SayByeMsg>;
using SayHelloMsg2Pair = core::PBMsgPair<3, proto::SayHelloMsg2>;
using SayByeMsg2Pair = core::PBMsgPair<4, proto::SayByeMsg2>;
using ChangeNameMsgPair = core::PBMsgPair<5, proto::ChangeNameMsg>;
using ChangeNameMsg2Pair = core::PBMsgPair<6, proto::ChangeNameMsg2>;
using RespSayByeMsgPair = core::PBMsgPair<7, proto::RespSayByeMsg>;
using RespSayByeMsg2Pair = core::PBMsgPair<8, proto::RespSayByeMsg2>;



class Stream :public core::ProtoBufStream<Stream, SayHelloMsgPair, SayByeMsgPair, ChangeNameMsgPair, RespSayByeMsgPair>
{
    std::string name;
public:
    core::StreamVariantErrcode on_change_msg_s(const std::shared_ptr<proto::SayHelloMsg>& hello,
                                               std::vector<uint8_t> &ret_buf)
    {
        printf("on_req 1 %s\n",hello->name().c_str());
        name = hello->name();
        core::ProtoBufMsg::write_msg<SayHelloMsgPair>(ret_buf,*hello);
        return core::StreamVariantErrcode::ok;
    }

    void on_read_msg_s(const std::shared_ptr<proto::ChangeNameMsg>& m)
    {
        printf("on_change 1 %s\n", m->name().c_str());
        name = m->name();
        this->setIsWaitPeerChangeRet(true);
        send<ChangeNameMsgPair>(*m);
    }
    mqas::core::StreamVariantErrcode on_peer_quit_msg_s(const std::shared_ptr<proto::SayByeMsg>& m,
                                      std::vector<uint8_t>& buf)
    {
        proto::RespSayByeMsg msg;
        msg.set_name(name);
        core::ProtoBufMsg::write_msg<RespSayByeMsgPair>(buf,msg);
        return mqas::core::StreamVariantErrcode::ok;
    }
};

class Stream2 :public core::ProtoBufStream<Stream2, SayHelloMsg2Pair, SayByeMsg2Pair, ChangeNameMsg2Pair, RespSayByeMsg2Pair>
{
    std::string name;
public:
    core::StreamVariantErrcode on_change_msg_s(const std::shared_ptr<proto::SayHelloMsg2>& hello,
        std::vector<uint8_t>& ret_buf)
    {
        printf("on_req 2 %s\n", hello->name().c_str());
        name = hello->name();
        core::ProtoBufMsg::write_msg<SayHelloMsg2Pair>(ret_buf, *hello);
        return core::StreamVariantErrcode::ok;
    }

    void on_read_msg_s(const std::shared_ptr<proto::ChangeNameMsg2>& m)
    {
        printf("on_change 2 %s\n", m->name().c_str());
        name = m->name();
        this->setIsWaitPeerChangeRet(true);
        send<ChangeNameMsg2Pair>(*m);
    }
    mqas::core::StreamVariantErrcode on_peer_quit_msg_s(const std::shared_ptr<proto::SayByeMsg2>& m,
        std::vector<uint8_t>& buf)
    {
        proto::RespSayByeMsg2 msg;
        msg.set_name(name);
        core::ProtoBufMsg::write_msg<RespSayByeMsg2Pair>(buf, msg);
        return mqas::core::StreamVariantErrcode::ok;
    }
};

int main(int argc,const char** argv)
{
	Context<core::InitFlags::GLOBAL_SERVER> context;
	io::Context io_cxt;
	core::engine_base<core::engine<core::Connect<core::StreamVariant<core::StreamVariantPair<1,Stream>,
        core::StreamVariantPair<2, Stream2>>>>> e(io_cxt);
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
