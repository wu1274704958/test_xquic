#include <mqas/context.h>
#include <mqas/io/context.h>
#include "mqas/core/engine_base.h"
#include <iostream>
#include <mqas/core/engine.h>
#include <mqas/core/connect.h>
#include <mqas/core/stream.h>
#include <mqas/tools/stream/relay_stream_client.h>
using namespace mqas;

using StreamTy = core::StreamVariant<core::StreamVariantPair<1,tools::RelayStreamClient>>;

MQAS_SHARE_EASYLOGGINGPP
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

    conn->make_stream([&io_cxt,&relay_ip,relay_port,relay_active,&relay_addr](std::weak_ptr<StreamTy> stream){
        auto s_ = stream.lock();
        tools::proto::relay::ReqRelay req;
		auto address = req.mutable_address();
		address->set_ip(relay_ip);
		address->set_port(relay_port);
        s_->req_change<tools::RelayStreamClient,tools::relay::ReqRelayPair>(req);
		s_->on_close_signal.connect([](std::shared_ptr<core::IStreamVariant>){
			printf("stream closed\n");	
			IsRunning() = false;
		});
		s_->on_change_stream_signal.connect([relay_active,&relay_addr](std::shared_ptr<core::IStreamVariant> s){
			auto relay = std::dynamic_pointer_cast<tools::RelayStreamClient>(s);
			relay->on_recv_signal.connect([](io::UdpSocket*, const std::optional<std::span<uint8_t>>& data, ssize_t nread, const sockaddr* addr, unsigned){
				if(data.has_value())
				{
					std::vector<uint8_t> buf;
					buf.resize(data.value().size() + 1,0);
					memcpy(buf.data(),data.value().data(),data.value().size());
					printf("recv %s\n",(const char*)buf.data());
				}
			});
			if(relay_active)
			{
				std::vector<std::span<uint8_t>> buf;
				std::string str = "hello!!!";
				buf.push_back( { (uint8_t*)str.c_str(), str.size() } );
				relay->try_send(buf,relay_addr);
			}
		});
    });
	io_cxt.run_until(IsRunning());
	
	return 0;
}
