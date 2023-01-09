#include <openssl/pem.h>
#include <openssl/x509.h>
#include <openssl/ssl.h>
#include <iostream>
#include <lsquic.h>
#include <platform.h>
#include <uv.h>

static bool IsRunning = true;
class Context
{
public:
	Context()
	{
		loop = std::make_shared<uv_loop_t>();
		uv_loop_init(loop.get());

		socket = std::make_shared<uv_udp_t>();
		uv_udp_init(loop.get(), socket.get());
		struct sockaddr_in recv_addr;
		uv_ip4_addr("0.0.0.0", 8083, &recv_addr);
		uv_udp_bind(socket.get(), (const struct sockaddr*)&recv_addr, UV_UDP_REUSEADDR);
		platform_handle(*socket);
	}
	~Context()
	{
		uv_loop_close(loop.get());
	}
	std::shared_ptr<uv_loop_t> loop;
	std::shared_ptr<uv_udp_t> socket;
	std::shared_ptr<uv_timer_t> proc_conns_timer;
};

void sig_handler(int sig)
{
	if (sig == SIGINT)
	{
		IsRunning = false;
	}
}
int tut_log_buf(void* ctx, const char* buf, size_t len) {
	std::string str(buf, len);
	std::cout << str << std::endl;
	return 0;
}
static SSL_CTX* s_ssl_ctx;

static int
load_cert(const char* cert_file, const char* key_file);

int packets_out(
	void* packets_out_ctx,
	const struct lsquic_out_spec* out_spec,
	unsigned                       n_packets_out
);
struct ssl_ctx_st* get_ssl_ctx(void* peer_ctx, const struct sockaddr* local);
lsquic_conn_ctx_t* on_new_conn(void* stream_if_ctx, lsquic_conn_t* c);
void on_conn_closed(lsquic_conn_t* c);
lsquic_stream_ctx_t* on_new_stream(void* stream_if_ctx, lsquic_stream_t* s);
void on_read(lsquic_stream_t* s, lsquic_stream_ctx_t* h);
void on_write(lsquic_stream_t* s, lsquic_stream_ctx_t* h);
void on_close(lsquic_stream_t* s, lsquic_stream_ctx_t* h);
void process_conns(lsquic_engine_t* engine, Context& cxt);
//main /////////////
int main(int argc, const char** argv)
{
	signal(SIGINT, sig_handler);
	if (0 != lsquic_global_init(LSQUIC_GLOBAL_SERVER))
	{
		printf("init lsquic failed!!!\n");
		return -1;
	}
	load_cert("cert-def.pem", "key-def.pem");

	static const struct lsquic_logger_if logger_if = { tut_log_buf, };

	lsquic_logger_init(&logger_if, nullptr, LLTS_HHMMSSUS);
	lsquic_set_log_level("warning");

	Context context;
	struct lsquic_stream_if stream_if = {};
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

	std::array<char, 1500> buffer;
	// 异步接收数据
	asio::ip::udp::endpoint sender_endpoint;

	process_conns(engine, context);

	printf("Default loop.\n");
	uv_run(context.loop.get(), UV_RUN_DEFAULT);

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
	//if (cxt.timer)cxt.timer->cancel();
	//int diff;
	//int timeout;
	//lsquic_engine_process_conns(engine);
	//if (lsquic_engine_earliest_adv_tick(engine, &diff)) {
	//	if (diff > 0)
	//		timeout = diff;   /* To seconds */
	//	else
	//		timeout = 0;
	//	cxt.timer = std::make_shared< asio::steady_timer>(cxt.io_context, std::chrono::microseconds(timeout));
	//	Context* cxt_ptr = &cxt;
	//	cxt.timer->async_wait([=](const asio::error_code& error) {
	//		if (!error)
	//			process_conns(engine, *cxt_ptr);
	//		});
	//}
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
			//cxt->socket->send_to(buffers, endpoint);
		}
		catch (asio::system_error e)
		{
			std::cerr << "packets_out " << endpoint << " err = " << e.what() << std::endl;
		}
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
	char buf[1500] = { 0 };
	std::string str;
	int len = 0;
	printf("begin read \n");
	while ((len = ::lsquic_stream_read(s, buf, sizeof(buf))) > 0)
	{
		str.append(buf, len);
	}
	::lsquic_stream_wantwrite(s, 1);
	::lsquic_stream_wantread(s, 0);
	printf("recv %zx str = %s\n", (size_t)s, str.c_str());
}
void on_write(lsquic_stream_t* s, lsquic_stream_ctx_t* h)
{
	const char* str = "Hi! client";
	::lsquic_stream_write(s, str, strlen(str));
	::lsquic_stream_flush(s);
	::lsquic_stream_wantwrite(s, 0);
}
void on_close(lsquic_stream_t* s, lsquic_stream_ctx_t* h)
{
	printf("stream closed %zx\n", (size_t)s);
}
static int select_alpn(SSL* ssl, const unsigned char** out, unsigned char* outlen,
	const unsigned char* in, unsigned int inlen, void* arg)
{
	std::cerr << "IN SELECT ALPN" << std::endl;
	int r;
	unsigned char server_alpns[] = {
		"\04echo"
	};
	r = SSL_select_next_proto((unsigned char**)out, outlen, in, inlen,
		(unsigned char*)server_alpns, sizeof(server_alpns) - 1);
	if (r == OPENSSL_NPN_NEGOTIATED)
		return SSL_TLSEXT_ERR_OK;
	else {
		fprintf(stderr, "no supported protocol can be selected from %.*s\n",
			(int)inlen, (char*)in);
		return SSL_TLSEXT_ERR_ALERT_FATAL;
	}
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
	SSL_CTX_set_alpn_select_cb(s_ssl_ctx, select_alpn, nullptr);
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
