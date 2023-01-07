#include <openssl/pem.h>
#include <openssl/x509.h>
#include <openssl/ssl.h>
#include <iostream>
#include <lsquic.h>
#include <asio.hpp>

static bool IsRunning = true;
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
    std::shared_ptr< asio::steady_timer> timer;
};

void sig_handler(int sig)
{
    if (sig == SIGINT)
    {
        IsRunning = false;
    }
}

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
void on_read(lsquic_stream_t* s, lsquic_stream_ctx_t* h);
void on_write(lsquic_stream_t* s, lsquic_stream_ctx_t* h);
void on_close(lsquic_stream_t* s, lsquic_stream_ctx_t* h);
void process_conns(lsquic_engine_t* engine, Context& cxt);
//main /////////////
int main(int argc,const char** argv)
{
    signal(SIGINT, sig_handler);
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
    stream_if.on_read = on_read;
    stream_if.on_write = on_write;
    stream_if.on_close = on_close;

    lsquic_engine_api engine_api = {};
    engine_api.ea_packets_out = packets_out;
    engine_api.ea_packets_out_ctx = &context;  /* For example */
    engine_api.ea_stream_if = &stream_if;
    engine_api.ea_stream_if_ctx = &context;
    engine_api.ea_get_ssl_ctx = get_ssl_ctx;
    lsquic_engine_t* engine = lsquic_engine_new(LSENG_SERVER, &engine_api);
    while (IsRunning)
    {
        std::array<char, 1500> buffer;
        // 异步接收数据
        asio::ip::udp::endpoint sender_endpoint;
        context.socket->async_receive_from(
            asio::buffer(buffer), sender_endpoint,
            [&](const asio::error_code& error, std::size_t bytes_transferred)
            {
                if (!error)
                {
                    std::cout << "Received " << bytes_transferred << " bytes.\n";
                    auto local = context.socket->local_endpoint();
                    int ret = ::lsquic_engine_packet_in(engine, reinterpret_cast<const unsigned char*>(buffer.data()), bytes_transferred,
                        local.data(), sender_endpoint.data(), nullptr, 0);
                    printf("lsquic_engine_packet_in %d", ret);
                    process_conns(engine,context);
                }
                else
                    std::cout << "Receive get error " << error.message() << std::endl;
            }
        );
        context.io_context.run_for(std::chrono::seconds(1));
    }
    
    printf("end\n");
    lsquic_global_cleanup();

    if (s_ssl_ctx)
    {
	    SSL_CTX_free(s_ssl_ctx);
        s_ssl_ctx = nullptr;
    }
    return 0;
}
void process_conns(lsquic_engine_t* engine, Context& cxt)
{
    if (cxt.timer)cxt.timer->cancel();
    int diff;
    int timeout;
    lsquic_engine_process_conns(engine);
    if (lsquic_engine_earliest_adv_tick(engine, &diff)) {
        if (diff > 0)
            timeout = diff / 1000;   /* To seconds */
        else
            timeout = 0;
        cxt.timer = std::make_shared< asio::steady_timer>(cxt.io_context, std::chrono::milliseconds(timeout));
        cxt.timer->async_wait([&](const asio::error_code& error) {
            if (!error)
                process_conns(engine, cxt);
        });
    }
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
    printf("stream create %zx\n", (size_t)s);
    return nullptr;
}
void on_read(lsquic_stream_t* s, lsquic_stream_ctx_t* h)
{
	char buf[1500] = {0};
    std::string str;
    int len = 0;
	while ((len = ::lsquic_stream_read(s, buf, sizeof(buf))) > 0)
	{
		str.append(buf,len);
	}
    ::lsquic_stream_wantwrite(s, 1);
    printf("recv %zx str = %s\n",(size_t)s, str.c_str());
}
void on_write(lsquic_stream_t* s, lsquic_stream_ctx_t* h)
{
    const char* str = "Hi! client";
    ::lsquic_stream_write(s, str, strlen(str));
}
void on_close(lsquic_stream_t* s, lsquic_stream_ctx_t* h)
{
    printf("stream closed %zx\n",(size_t)s);
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
