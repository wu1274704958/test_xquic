#include <mqas/context.h>
#include <mqas/io/context.h>
#include "mqas/core/engine_base.h"
#include <iostream>
#include <mqas/core/engine.h>
#include <mqas/core/connect.h>
#include <mqas/core/pb_stream.h>
#include <mqas/comm/locator.h>
#include <mqas/tools/stream/relay_stream.h>
#include <mqas/tools/model/relay_model.h>
#include <mqas/comm/engine.h>

using namespace mqas;



int main(int argc,const char** argv)
{
	Context<core::InitFlags::GLOBAL_SERVER> context;
	io::Context io_cxt;

	comm::locator::inst()->deposit<mqas::tools::relay::relay_model>();

	auto e = mqas::comm::engine_util::launch_sub_engine<core::StreamVariant<
		core::StreamVariantPair<1, mqas::tools::RelayStream>>,
		core::Connect,core::engine,core::engine_base>(io_cxt,"conf.txt",core::EngineFlags::Server);

	io_cxt.run_until(IsRunning());
	return 0;
}
