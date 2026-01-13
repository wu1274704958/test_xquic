#include <mqas/context.h>
#include <mqas/io/context.h>
#include "mqas/core/engine_base.h"
#include <iostream>
#include <mqas/core/engine.h>
#include <mqas/core/connect.h>
#include <mqas/core/stream.h>
#include <mqas/tools/stream/relay_stream_client.h>
#include <mqas/comm/binary.hpp>
#include <boost/uuid/uuid.hpp>
using namespace mqas;

using StreamTy = core::StreamVariant<core::StreamVariantPair<1,tools::RelayStreamClient>>;


int main(int argc,const char** argv)
{
	Context<core::InitFlags::GLOBAL_CLIENT> context;
	io::Context io_cxt;
	core::engine_base<core::engine<core::Connect<StreamTy>>> e(io_cxt);
	try{
		if(argc > 1)
			e.init(argv[1],core::EngineFlags::None);
		else
			e.init("conf.txt",core::EngineFlags::None);
		e.start_recv();
		e.process_conns();
	}catch (std::exception& e)
	{
		std::cerr << e.what() << std::endl;
	}

	sockaddr addr{};
	auto ip = toml::find<std::string>(*e.get_engine()->get_config(),"client", "ip");
	auto port = toml::find<int>(*e.get_engine()->get_config(), "client", "port");
	if(!io::Ip::str2addr_ipv4(ip.c_str(), port, addr))
		throw new std::exception("Not found target address!");
    auto c = e.get_engine()->connect(addr,N_LSQVER);
    auto conn = c.lock();

	std::string relay_ip = toml::find<std::string>(*e.get_engine()->get_config(),"relay", "ip");
	auto relay_port = toml::find<int>(*e.get_engine()->get_config(), "relay", "port");
	auto relay_active = toml::find<bool>(*e.get_engine()->get_config(), "relay", "active");
	sockaddr relay_addr{};
	if(!io::Ip::str2addr_ipv4(relay_ip.c_str(), relay_port, relay_addr))
		throw new std::exception("Not found relay address!");

    auto stream = conn->make_stream();

    tools::proto::relay::ReqRelay req;
    boost::uuids::uuid token;
    std::memset(&token,0,sizeof(boost::uuids::uuid));
    token.data[0] = 1;
    req.mutable_token()->set_data((const char*)&token.data, token.size());
    stream->req_change<tools::RelayStreamClient,tools::relay::ReqRelayPair>(req);
    stream->on_close_signal.connect([](std::shared_ptr<core::IStreamVariant>){
        printf("stream closed\n");
        IsRunning() = false;
    });

    auto relay = stream->get_holds_stream<tools::RelayStreamClient>();
    relay->on_recv_signal.connect([relay,&relay_addr](io::UdpSocket*, const std::optional<std::span<uint8_t>>& data, ssize_t nread, const sockaddr* addr, unsigned){
        if(data.has_value())
        {
            auto num = comm::from_big_endian<uint32_t>(data.value());
            printf("recv %d\n",num);

            std::array<uint8_t,4> buf;
            comm::to_big_endian(num + 1,buf);
            relay->try_send(buf,relay_addr);
        }
    });
    if(relay_active)
    {
        relay->on_ready.connect([relay,&relay_addr](){
            std::array<uint8_t,4> buf;
            comm::to_big_endian(1,buf);
            relay->try_send(buf,relay_addr);
        });
    }

	io_cxt.run_until(IsRunning());
	
	return 0;
}
