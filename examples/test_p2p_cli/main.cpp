#include <mqas/context.h>
#include <mqas/io/context.h>
#include "mqas/core/engine_base.h"
#include <iostream>
#include <mqas/core/engine.h>
#include <mqas/core/connect.h>
#include <mqas/core/pb_stream.h>
#include <mqas/tools/stream/p2p_lobby_client.h>
#include <mqas/tools/proto/p2p.pb.h>
using namespace mqas;
MQAS_SHARE_EASYLOGGINGPP


class LobbyStream : public tools::p2p::P2PLobbyClientStream {
protected:
	void on_get_peer_list() override
	{
		print_peer_list();
	}
	void print_peer_list() const
	{
		printf("peer list:\n");
		if (peer_list == nullptr)
			return;
		for (int i = 0; i < peer_list->peer_list_size(); ++i)
		{
			auto it = peer_list->peer_list().Get(i);
			std::cout << it.id() << "\t------\t" << it.name() << std::endl;
		}
	}
};

using StreamType = core::StreamVariant<
	core::StreamVariantPair<1, LobbyStream>>;

int main(int argc, const char** argv)
{
	Context<core::InitFlags::GLOBAL_CLIENT> context;
	io::Context io_cxt;
	core::engine_base<core::engine<core::Connect<StreamType>>> e(io_cxt);
	try {
		e.init("conf.txt", core::EngineFlags::None);
		e.start_recv();
		e.process_conns();
		sockaddr addr{};
		io::Ip::str2addr_ipv4("127.0.0.1", 8084, addr);
		auto c = e.get_engine()->connect(addr, N_LSQVER);
		auto conn = c.lock();
		auto t = io_cxt.make_handle<io::Timer>();
		std::weak_ptr<StreamType> stream_out;
		conn->make_stream([&io_cxt, &stream_out](std::weak_ptr<StreamType> stream) {
			stream_out = stream;
			auto s = stream.lock();
			mqas::tools::proto::p2p::ReqRegistePeer msg;
			s->req_change<LobbyStream, mqas::tools::p2p::ReqRegistePeerPair>(msg);
			auto lobby_stream = s->get_holds_stream<LobbyStream>();
			lobby_stream->on_change_helper = [stream](const mqas::tools::proto::p2p::ReqRespondPeerReqConnect& msg)
			{
				auto s = stream.lock();
				s->req_change<LobbyStream, mqas::tools::p2p::ReqRespondPeerReqConnectPair>(msg);
			};
			lobby_stream->on_change_helper_by_req = [stream](const mqas::tools::proto::p2p::ReqConnectPeer& msg)
			{

			};
		});
		t->start([&stream_out](mqas::io::Timer* t) {
			auto s = stream_out.lock();
			if (s && s->has_holds_stream())
			{
				s->get_holds_stream<LobbyStream>()->req_peer_list();
			}
		}, 1000, 5000);
	}
	catch (std::exception& e)
	{
		std::cerr << e.what() << std::endl;
	}
	io_cxt.run_until(IsRunning());
	return 0;
}
