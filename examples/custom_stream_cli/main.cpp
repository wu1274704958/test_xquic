#include <mqas/context.h>
#include <mqas/io/context.h>
#include "mqas/core/engine_base.h"
#include <iostream>
#include <mqas/core/engine.h>
#include <mqas/core/connect.h>
#include <mqas/core/stream.h>
using namespace mqas;

class Stream:public core::IStream
{
public:
    static constexpr size_t STREAM_TAG = 1;

    void on_init(::lsquic_stream_t *lsquic_stream, core::connect_cxt *connect_cxt) {
        IStream::on_init(lsquic_stream, connect_cxt);
        std::string_view sv("hello!!!");
        write({reinterpret_cast<uint8_t*>((char*)sv.data()),sv.size()});
    }

    size_t on_read(const std::span<const uint8_t> &current) {
        std::string_view sv(reinterpret_cast<const char*>(current.data()), current.size());
        std::cout << sv <<std::endl;
        return current.size();
    }
};

int main(int argc,const char** argv)
{
	Context<core::InitFlags::GLOBAL_CLIENT> context;
	io::Context io_cxt;
	core::engine_base<core::engine<core::Connect<core::StreamVariant<Stream>>>> e(io_cxt);
	try{
		e.init("conf.txt",core::EngineFlags::None);
		e.start_recv();
		e.process_conns();
        sockaddr addr{};
        io::Ip::str2addr_ipv4("127.0.0.1",8084,addr);
        auto c = e.get_engine()->connect(addr,N_LSQVER);
        auto conn = c.lock();
        uv_tty_t tty;
        conn->make_stream([&io_cxt,&tty](std::weak_ptr<core::StreamVariant<Stream>> stream){
            auto s_ = stream.lock();
            s_->req_change<Stream>();
        });
	}catch (std::exception& e)
	{
		std::cerr << e.what() << std::endl;
	}
	io_cxt.run_until(IsRunning());
	return 0;
}
