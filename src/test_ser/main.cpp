#include <iostream>
#include <lsquic.h>

int packets_out(
    void* packets_out_ctx,
    const struct lsquic_out_spec* out_spec,
    unsigned                       n_packets_out
    );

int main(int argc,const char** argv)
{
    if (0 != lsquic_global_init(LSQUIC_GLOBAL_SERVER))
    {
        printf("init lsquic failed!!!\n");
        return -1;
    }
    lsquic_engine_api engine_api = {
        .ea_packets_out = packets_out,
        .ea_packets_out_ctx = nullptr,  /* For example */
        .ea_stream_if = &stream_callbacks,
        .ea_stream_if_ctx = &some_context,
    };
    lsquic_engine_t* engine = lsquic_engine_new(LSENG_SERVER, &engine_api);
    lsquic_global_cleanup();
    return 0;
}

int packets_out(
    void* packets_out_ctx,
    const struct lsquic_out_spec* out_spec,
    unsigned n_packets_out
)
{

}
