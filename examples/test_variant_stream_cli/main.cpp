#include <mqas/context.h>
#include <mqas/io/context.h>
#include "mqas/core/engine_base.h"
#include <iostream>
#include <mqas/core/engine.h>
#include <mqas/core/connect.h>
#include <mqas/core/pb_stream.h>
#include "say_hello.pb.h"
using namespace mqas;

using SayHelloMsgPair = core::PBMsgPair<1,proto::SayHelloMsg>;
using SayByeMsgPair = core::PBMsgPair<2,proto::SayByeMsg>;
using SayHelloMsg2Pair = core::PBMsgPair<3, proto::SayHelloMsg2>;
using SayByeMsg2Pair = core::PBMsgPair<4, proto::SayByeMsg2>;
using ChangeNameMsgPair = core::PBMsgPair<5, proto::ChangeNameMsg>;
using ChangeNameMsg2Pair = core::PBMsgPair<6, proto::ChangeNameMsg2>;
using RespSayByeMsgPair = core::PBMsgPair<7, proto::RespSayByeMsg>;
using RespSayByeMsg2Pair = core::PBMsgPair<8, proto::RespSayByeMsg2>;


class Stream2 :public core::ProtoBufStream<Stream2, SayHelloMsg2Pair, SayByeMsg2Pair, ChangeNameMsg2Pair, RespSayByeMsg2Pair>
{
public:
    core::StreamVariantErrcode on_change_msg_s(const std::shared_ptr<proto::SayHelloMsg2>& hello,
        std::vector<uint8_t>& ret_buf)
    {
        hello->set_name("hi");
        core::ProtoBufMsg::write_msg<SayHelloMsg2Pair>(ret_buf, *hello);
        return core::StreamVariantErrcode::ok;
    }
    void on_peer_change_ack_msg_s(mqas::core::StreamVariantErrcode code, const std::shared_ptr<proto::SayHelloMsg2>& m)
    {
        printf("change to hi2\n");
        proto::ChangeNameMsg2 changeMsg;
        changeMsg.set_name("hi2");
        send <ChangeNameMsg2Pair>(changeMsg);

        proto::SayByeMsg2 quit_msg;
        std::vector<uint8_t> buf;
        if (core::ProtoBufMsg::write_msg<SayByeMsg2Pair>(buf, quit_msg))
            assert(req_quit(buf));
    }
    mqas::core::StreamVariantErrcode on_peer_quit_ack_msg_s(mqas::core::StreamVariantErrcode code, const std::shared_ptr<proto::RespSayByeMsg2>& m)
    {
        printf("stream2 on_peer_quit  %s\n", m->name().c_str());
        return mqas::core::StreamVariantErrcode::ok;
    }
};

class Stream:public core::ProtoBufStream<Stream,SayHelloMsgPair,SayByeMsgPair,ChangeNameMsgPair,RespSayByeMsgPair, SayHelloMsg2Pair>
{
public:
    core::StreamVariantErrcode on_change_msg_s(const std::shared_ptr<proto::SayHelloMsg>& hello,
                                               std::vector<uint8_t> &ret_buf)
    {
        hello->set_name("hello");
        core::ProtoBufMsg::write_msg<SayHelloMsgPair>(ret_buf,*hello);
        return core::StreamVariantErrcode::ok;
    }

    void on_peer_change_ack_msg_s(mqas::core::StreamVariantErrcode code, const std::shared_ptr<proto::SayHelloMsg>& m);
    
    mqas::core::StreamVariantErrcode on_peer_quit_ack_msg_s(mqas::core::StreamVariantErrcode code,const std::shared_ptr<proto::RespSayByeMsg>& m)
    {
        printf("stream1 on_peer_quit  %s\n", m->name().c_str());
        return mqas::core::StreamVariantErrcode::ok;
    }

    void on_resume()
    {
        printf("stream1 on_resume\n");
        proto::SayByeMsg quit_msg;
        std::vector<uint8_t> buf;
        if (core::ProtoBufMsg::write_msg<SayByeMsgPair>(buf, quit_msg))
            assert(req_quit(buf));
    }
    void on_pause()
    {
        printf("stream1 on_pause\n");
    }
};

using StreamType = core::StreamVariant<core::StreamVariantPair<1, Stream>, core::StreamVariantPair<2, Stream2>>;


void Stream::on_peer_change_ack_msg_s(mqas::core::StreamVariantErrcode code, const std::shared_ptr<proto::SayHelloMsg>& m)
{
    printf("change to hello1\n");
    proto::ChangeNameMsg changeMsg;
    changeMsg.set_name("hello1");
    send <ChangeNameMsgPair>(changeMsg);

    auto out = get_outer();
    auto stream_out = std::dynamic_pointer_cast<StreamType>(out);
    proto::SayHelloMsg2 msg;
    stream_out->req_change<Stream2, SayHelloMsg2Pair>(msg);
}

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
        });
        t->start([&stream_out](io::Timer* t){
            auto s = stream_out.lock();
            if(s && !s->has_holds_stream())
            {
                proto::SayHelloMsg msg;
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
