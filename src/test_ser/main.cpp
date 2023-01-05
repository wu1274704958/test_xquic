#include <iostream>
#include <xquic/xquic.h>
#include <format>
#include <platform.h>

class XqcServerDef;

int main(int argc,const char** argv)
{
    xqc_platform_init_env();

    xqc_config_t config = {};
    xqc_engine_get_default_config(&config,xqc_engine_type_t::XQC_ENGINE_CLIENT);

    xqc_engine_ssl_config_t ssl_cfg = {};



    return 0;
}


class XqcServerDef
{
	
};