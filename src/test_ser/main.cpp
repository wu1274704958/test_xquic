#include <openssl/pem.h>
#include <openssl/x509.h>
#include <openssl/ssl.h>
#include <iostream>
#include <lsquic.h>
#include <asio.hpp>


class Context
{
public:
    Context()
    {
        socket = std::make_shared<asio::ip::udp::socket>(io_context);
        socket->open(asio::ip::udp::v4());
        socket->bind(asio::ip::udp::endpoint(asio::ip::address::from_string("0.0.0.0"),8083));
    }
    asio::io_context io_context;
    std::shared_ptr<asio::ip::udp::socket> socket;
};
class Stream

static SSL_CTX* s_ssl_ctx;

static int
load_cert(const char* cert_file, const char* key_file);

int packets_out(
    void* packets_out_ctx,
    const struct lsquic_out_spec* out_spec,
    unsigned                       n_packets_out
    );
struct ssl_ctx_st* get_ssl_ctx(void* peer_ctx,const struct sockaddr* local);
lsquic_conn_ctx_t* on_new_conn(void* stream_if_ctx,lsquic_conn_t* c);
void on_conn_closed(lsquic_conn_t* c);
lsquic_stream_ctx_t* on_new_stream(void* stream_if_ctx, lsquic_stream_t* s);
//main /////////////
int main(int argc,const char** argv)
{
    if (0 != lsquic_global_init(LSQUIC_GLOBAL_SERVER))
    {
        printf("init lsquic failed!!!\n");
        return -1;
    }
    load_cert("cert-def.pem","key-def.pem");
    Context context;
    struct lsquic_stream_if stream_if ={};
    stream_if.on_new_conn = on_new_conn;
    stream_if.on_conn_closed = on_conn_closed;
    stream_if.on_new_stream = on_new_stream;

    lsquic_engine_api engine_api = {};
    engine_api.ea_packets_out = packets_out;
    engine_api.ea_packets_out_ctx = &context;  /* For example */
    engine_api.ea_stream_if = &stream_if;
    engine_api.ea_stream_if_ctx = &context;
    engine_api.ea_get_ssl_ctx = get_ssl_ctx;
    //lsquic_engine_t* engine = lsquic_engine_new(LSENG_SERVER, &engine_api);
    context.io_context.run();
    lsquic_global_cleanup();

    if (s_ssl_ctx)
    {
	    SSL_CTX_free(s_ssl_ctx);
        s_ssl_ctx = nullptr;
    }
    return 0;
}

int packets_out(
    void* packets_out_ctx,
    const struct lsquic_out_spec* out_spec,
    unsigned n_packets_out
)
{
	auto cxt = reinterpret_cast<Context*>(packets_out_ctx);
    
    for (int n = 0; n < n_packets_out; ++n)
    {
    	asio::ip::address addr(asio::ip::make_address_v4(ntohl(*(u_long*)(out_spec[n].dest_sa->sa_data + 2))));
        u_short port = ntohs(*(u_short*)out_spec[n].dest_sa->sa_data);
        asio::ip::udp::endpoint endpoint(addr, port);

        asio::mutable_buffers_1 buffer(out_spec[n].iov->iov_base, out_spec[n].iov->iov_len);
    	cxt->socket->send_to(buffer, endpoint);
    }
    
	return n_packets_out;
}

struct ssl_ctx_st* get_ssl_ctx(void* peer_ctx,
    const struct sockaddr* local
)
{
	return s_ssl_ctx;
}
lsquic_conn_ctx_t* on_new_conn(void* stream_if_ctx,
    lsquic_conn_t* c)
{
    printf("Create new Connect\n");
	return nullptr;
}
void on_conn_closed(lsquic_conn_t* c)
{
    printf("Connect closed\n");
}
lsquic_stream_ctx_t* on_new_stream(void* stream_if_ctx, lsquic_stream_t* s)
{
    ::lsquic_stream_wantread(s, 1);
}
static int load_cert(const char* cert_file, const char* key_file)
{
    int rv = -1;
    int ret = 0;
    s_ssl_ctx = SSL_CTX_new(TLS_method());
    if (!s_ssl_ctx)
    {
        printf("SSL_CTX_new failed\n");
        goto end;
    }
    SSL_CTX_set_min_proto_version(s_ssl_ctx, TLS1_3_VERSION);
    SSL_CTX_set_max_proto_version(s_ssl_ctx, TLS1_3_VERSION);
    SSL_CTX_set_default_verify_paths(s_ssl_ctx);
    
	if ((ret = SSL_CTX_use_certificate_chain_file(s_ssl_ctx, cert_file)) != 1)
    {
        printf("SSL_CTX_use_certificate_chain_file failed ");
        goto end;
    }
    if (1 != SSL_CTX_use_PrivateKey_file(s_ssl_ctx, key_file,
        SSL_FILETYPE_PEM))
    {
        printf("SSL_CTX_use_PrivateKey_file failed");
        goto end;
    }
    rv = 0;

end:
    if (rv != 0)
    {
        if (s_ssl_ctx)
            SSL_CTX_free(s_ssl_ctx);
        s_ssl_ctx = nullptr;
    }
    return rv;
}
