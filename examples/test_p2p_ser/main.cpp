#include <mqas/context.h>
#include <mqas/io/context.h>
#include "mqas/core/engine_base.h"
#include <iostream>
#include <mqas/core/engine.h>
#include <mqas/core/connect.h>
#include <mqas/core/pb_stream.h>
#include <mqas/comm/locator.h>
#include <mqas/tools/stream/p2p_lobby.h>
#include <mqas/tools/model/p2p_model.h>

using namespace mqas;
MQAS_SHARE_EASYLOGGINGPP


int main(int argc,const char** argv)
{
	mqas::tools::p2p::p2p_model model;
	model.test_step_cxt();
	return 0;
}
