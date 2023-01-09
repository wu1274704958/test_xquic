#include <openssl/pem.h>
#include <openssl/x509.h>
#include <openssl/ssl.h>
#include <iostream>
#include <lsquic.h>
#include <asio.hpp>
#include <platform.h>

static bool IsRunning = true;
class Context
{
public:
    Context()
    {
        socket = std::make_shared<asio::ip::udp::socket>(io_context);
        socket->open(asio::ip::udp::v4());
        platform_handle(*socket);
        socket->bind(asio::ip::udp::endpoint(asio::ip::address::from_string("127.0.0.1"), 8084));
    }
    ~Context()
    {
        socket->close();
    }
    asio::io_context io_context;
    std::shared_ptr<asio::ip::udp::socket> socket;
    std::shared_ptr< asio::steady_timer> timer;
    lsquic_engine_t* engine = nullptr;
    lsquic_stream_t* stream = nullptr;
};

void sig_handler(int sig)
{
    if (sig == SIGINT)
    {
        IsRunning = false;
    }
}

int packets_out(
    void* packets_out_ctx,
    const struct lsquic_out_spec* out_spec,
    unsigned                       n_packets_out
);
lsquic_conn_ctx_t* on_new_conn(void* stream_if_ctx, lsquic_conn_t* c);
void on_conn_closed(lsquic_conn_t* c);
lsquic_stream_ctx_t* on_new_stream(void* stream_if_ctx, lsquic_stream_t* s);
void on_read(lsquic_stream_t* s, lsquic_stream_ctx_t* h);
void on_write(lsquic_stream_t* s, lsquic_stream_ctx_t* h);
void on_close(lsquic_stream_t* s, lsquic_stream_ctx_t* h);
void process_conns(Context& cxt);
void on_hsk_done(lsquic_conn_t* c, enum lsquic_hsk_status s);
static int
tut_log_buf(void* ctx, const char* buf, size_t len) {
    std::string str(buf, len);
    std::cout << str << std::endl;
    return 0;
}
//main /////////////
int main(int argc, const char** argv)
{
    signal(SIGINT, sig_handler);
    if (0 != lsquic_global_init(LSQUIC_GLOBAL_CLIENT))
    {
        printf("init lsquic failed!!!\n");
        return -1;
    }
    Context context;
    struct lsquic_stream_if stream_if = {};
    stream_if.on_new_conn = on_new_conn;
    stream_if.on_conn_closed = on_conn_closed;
    stream_if.on_new_stream = on_new_stream;
    stream_if.on_read = on_read;
    stream_if.on_write = on_write;
    stream_if.on_close = on_close;
    stream_if.on_hsk_done = on_hsk_done;

    lsquic_engine_api engine_api = {};
    engine_api.ea_packets_out = packets_out;
    engine_api.ea_packets_out_ctx = &context;  /* For example */
    engine_api.ea_stream_if = &stream_if;
    engine_api.ea_stream_if_ctx = &context;
    engine_api.ea_alpn = "echo";
    lsquic_engine_t* engine = lsquic_engine_new(0, &engine_api);

    context.engine = engine;
    
    static const struct lsquic_logger_if logger_if = { tut_log_buf, };

    lsquic_logger_init(&logger_if, nullptr, LLTS_HHMMSSUS);
    lsquic_set_log_level("warning");

    auto local_addr = context.socket->local_endpoint();
    asio::ip::udp::endpoint peer_addr = asio::ip::udp::endpoint(asio::ip::address::from_string("127.0.0.1"), 8083);

    auto conn = ::lsquic_engine_connect(engine, N_LSQVER, local_addr.data(), peer_addr.data(), nullptr, nullptr, nullptr, 0, nullptr, 0, nullptr, 0);
    printf("conn = %zx engine = %zx\n", (size_t)conn,(size_t)engine);
    process_conns(context);

    bool is_receive = false;
    std::array<char, 1500> buffer;
    // 异步接收数据
    asio::ip::udp::endpoint sender_endpoint;
    while (IsRunning)
    {
        if (!is_receive)
        {
            //printf("async_receive_from\n");
            context.socket->async_receive_from(
                asio::buffer(buffer), sender_endpoint,
                [&](const asio::error_code& error, std::size_t bytes_transferred)
                {
                    is_receive = false;
                    if (!error)
                    {
                        //std::cout << "Received " << bytes_transferred << " bytes.\n";
                        auto local = context.socket->local_endpoint();
                        int ret = ::lsquic_engine_packet_in(engine, reinterpret_cast<const unsigned char*>(buffer.data()), bytes_transferred,
                            local.data(), sender_endpoint.data(), nullptr, 0);
                        //printf("lsquic_engine_packet_in %d\n", ret);
                        process_conns(context);
                    }
                    else {
                        std::cout << "Receive get error " << error.message() << std::endl;
                        IsRunning = false;
                    }
                }
            );
            is_receive = true;
        }
        context.io_context.run_for(std::chrono::milliseconds(1));
    }
    printf("end===================================\n");
    if (context.stream)
        ::lsquic_stream_close(context.stream);
    ::lsquic_conn_close(conn);
    process_conns(context);
    lsquic_global_cleanup();

    return 0;
}
void process_conns(Context& cxt)
{
    if (cxt.timer)cxt.timer->cancel();
    int diff;
    int timeout;
    lsquic_engine_process_conns(cxt.engine);
    if (lsquic_engine_earliest_adv_tick(cxt.engine, &diff)) {
        if (diff > 0)
            timeout = diff ;   /* To seconds */
        else
            timeout = 0;
        cxt.timer = std::make_shared< asio::steady_timer>(cxt.io_context, std::chrono::microseconds(timeout));
        Context* cxt_ptr = &cxt;
        cxt.timer->async_wait([=](const asio::error_code& error) {
            if (!error)
                process_conns(*cxt_ptr);
            });
    }
}
void async_process_conns(Context& cxt)
{
    if (cxt.timer)cxt.timer->cancel();
    cxt.timer = std::make_shared< asio::steady_timer>(cxt.io_context, std::chrono::microseconds(0));
    Context* cxt_ptr = &cxt;
    cxt.timer->async_wait([=](const asio::error_code& error) {
        if (!error)
        process_conns(*cxt_ptr);
    });
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
        
        std::vector<asio::const_buffer> buffers;
        for (size_t i = 0; i < out_spec[n].iovlen; ++i)
            buffers.emplace_back(asio::const_buffer(out_spec[n].iov[i].iov_base, out_spec[n].iov[i].iov_len));
        try {
            cxt->socket->send_to(buffers, endpoint);
        }
        catch (asio::system_error e)
        {
            std::cerr << "packets_out "<< endpoint <<  " err = " << e.what() << std::endl;
        }
    }
    return n_packets_out;
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
    auto cxt = reinterpret_cast<Context*>(stream_if_ctx);
    cxt->stream = s;
    ::lsquic_stream_wantwrite(s, 1);
    printf("stream create %zx\n", (size_t)s);
    return reinterpret_cast<lsquic_stream_ctx_t*>(stream_if_ctx);
}
void on_read(lsquic_stream_t* s, lsquic_stream_ctx_t* h)
{
    auto cxt = reinterpret_cast<Context*>(h);
    char buf[1500] = { 0 };
    std::string str;
    int len = 0;
    printf("begin read \n");
    while ((len = ::lsquic_stream_read(s, buf, sizeof(buf))) > 0)
    {
        str.append(buf, len);
    }
    printf("recv %zx str = %s\n", (size_t)s, str.c_str());
    ::lsquic_stream_wantread(s, 0);
}
void on_write(lsquic_stream_t* s, lsquic_stream_ctx_t* h)
{
    const char* str = "Hi! server";
    printf("begin write \n");
    ::lsquic_stream_write(s, str, strlen(str));
    ::lsquic_stream_flush(s);
    ::lsquic_stream_wantread(s, 1);
    ::lsquic_stream_wantwrite(s, 0);
;}
void on_close(lsquic_stream_t* s, lsquic_stream_ctx_t* h)
{
    printf("stream closed %zx\n", (size_t)s);
    auto cxt = reinterpret_cast<Context*>(h);
    cxt->stream = nullptr;
}
void on_hsk_done(lsquic_conn_t* c, enum lsquic_hsk_status s)
{
    switch (s) {
    case LSQ_HSK_OK:
    case LSQ_HSK_RESUMED_OK:
        printf("handshake successful, start stdin watcher\n");
        break;
    default:
        printf("handshake failed\n");
        break;
    }

    lsquic_conn_make_stream(c);
}