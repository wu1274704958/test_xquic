#include <mqas/context.h>
#include <mqas/io/context.h>
#include "mqas/core/engine_base.h"
#include <iostream>
#include <mqas/core/engine.h>
#include <mqas/core/connect.h>
#include <mqas/core/stream.h>
using namespace mqas;
MQAS_SHARE_EASYLOGGINGPP
class Stream:public core::IStreamVariant
{
public:
    static constexpr size_t STREAM_TAG = 1;

    size_t on_read(const std::span<const uint8_t> &current) {
        std::string_view sv(reinterpret_cast<const char*>(current.data()), current.size());
        std::cout << sv <<std::endl;
        std::string_view sv2("copy that!!!");
        write({reinterpret_cast<uint8_t*>((char*)sv2.data()),sv2.size()});
        return current.size();
    }
    mqas::core::StreamVariantErrcode on_change(const std::span<uint8_t>& params,
                                                           std::vector<uint8_t>& ret_buf)
    {
        std::string_view sv(reinterpret_cast<const char*>(params.data()), params.size());
        std::cout << "on_change " << sv <<std::endl;
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
