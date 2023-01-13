#include <openssl/pem.h>
#include <openssl/x509.h>
#include <openssl/ssl.h>
#include <iostream>
#include <lsquic.h>
#include <platform.h>
#include <uv.h>
#include <vector>

void print_uv_err(int e, const char* msg);
static bool IsRunning = true;
class Context
{
public:
	Context()
	{

	}
	void init()
	{
		loop = std::make_shared<uv_loop_t>();
		uv_loop_init(loop.get());

		socket = std::make_shared<uv_udp_t>();
		uv_udp_init(loop.get(), socket.get());
		struct sockaddr_in recv_addr;
		uv_ip4_addr("0.0.0.0", 8083, &recv_addr);
		uv_udp_bind(socket.get(), (const struct sockaddr*)&recv_addr, UV_UDP_REUSEADDR);
		platform_handle(*socket);

		int addrlen = sizeof(sockaddr);
		int ret = uv_udp_getsockname(socket.get(), &local_addr, &addrlen);
		if (ret) print_uv_err(ret, "get local addr");
		socket->data = this;
		proc_conns_timer = std::make_shared<uv_timer_t>();
		ret = uv_timer_init(loop.get(), proc_conns_timer.get());
		if (ret) print_uv_err(ret, "init proc_conns_timer");
		proc_conns_timer->data = this;
	}
	void close_loop()
	{
		if(!loop)return;
		uv_loop_close(loop.get());
		loop.reset();
	}
	~Context()
	{
		close_loop();
	}
	std::shared_ptr<uv_loop_t> loop;
	std::shared_ptr<uv_udp_t> socket;
	std::shared_ptr<uv_timer_t> proc_conns_timer;
	lsquic_engine_t* engine;
	std::vector<char> recv_buf;
	struct sockaddr local_addr;
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
void process_conns( Context& cxt);
void print_uv_err(int e, const char* msg)
{
	printf("%s Got error = %s\n", msg, uv_strerror(e));
}
void alloc_cb(uv_handle_t* handle, size_t size, uv_buf_t* buf) {
	auto cxt = reinterpret_cast<Context*>(handle->data);
	if (!cxt)return;
	if(size > cxt->recv_buf.size())
		cxt->recv_buf.resize(size);
	buf->base = cxt->recv_buf.data();
	buf->len = cxt->recv_buf.size();
}
void recv_cb(uv_udp_t* req, ssize_t nread, const uv_buf_t* buf, const struct sockaddr* addr, unsigned int flags) {
	if (nread == 0 || addr == nullptr)
		return;
	if (nread < 0) {
		// there seems to be no way to get an error code here (none of the udp tests do)
		printf("recv error unexpected %zd\n",nread);
		uv_close((uv_handle_t*)req, NULL);
		return;
	}
	char sender[17] = { 0 };
	uv_ip4_name((struct sockaddr_in*)addr, sender, 16);
	fprintf(stderr, "recv from %s %zd bytes\n", sender, nread);
	auto cxt = reinterpret_cast<Context*>(req->data);
	if(!cxt)return;
	
	int ret = ::lsquic_engine_packet_in(cxt->engine, reinterpret_cast<const unsigned char*>(buf->base), nread,
		&cxt->local_addr, addr, nullptr, 0);
	printf("lsquic_engine_packet_in %d\n", ret);
	process_conns(*cxt);
}
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
	context.init();
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
	context.engine = engine;
	auto idler = std::make_shared<uv_idle_t>();
	process_conns(context);
	int ret = uv_udp_recv_start(context.socket.get(), alloc_cb, recv_cb);
	if(ret)
	{
		print_uv_err(ret,"start recv");
		goto END;
	}

	
	idler->data = context.loop.get();
	uv_idle_init(context.loop.get(), idler.get());
	uv_idle_start(idler.get(), [](uv_idle_t* handle)
	{
		if(!IsRunning)
			uv_stop(reinterpret_cast<uv_loop_t*>(handle->data));
	});
	
	printf("Default loop.\n");
	uv_run(context.loop.get(), UV_RUN_DEFAULT);
	
	END:
	printf("end\n");
	lsquic_global_cleanup();

	if (s_ssl_ctx)
	{
		SSL_CTX_free(s_ssl_ctx);
		s_ssl_ctx = nullptr;
	}
	return 0;
}
void process_conns(Context& cxt)
{
	if (cxt.proc_conns_timer)
		uv_timer_stop(cxt.proc_conns_timer.get());
	int diff;
	int timeout;
	lsquic_engine_process_conns(cxt.engine);
	if (lsquic_engine_earliest_adv_tick(cxt.engine, &diff)) {
		if (diff >= LSQUIC_DF_CLOCK_GRANULARITY)
			/* Expected case: convert to millisecond */
			timeout = diff / 1000;
		else if (diff <= 0)
			/* It should not happen often that the next tick is in the past
			 * as we just processed connections.  Avoid a busy loop by
			 * scheduling an event:
			 */
			timeout = 0.0;
		else
			/* Round up to granularity */
			timeout = LSQUIC_DF_CLOCK_GRANULARITY / 1000;
		int ret = uv_timer_start(cxt.proc_conns_timer.get(),[](uv_timer_t* handle)
		{
			auto cxt_p = reinterpret_cast<Context*>( handle->data);
			process_conns(*cxt_p);
		},timeout,0);
		if (ret)print_uv_err(ret, "start proc conns timer");
	}
}

int packets_out(
	void* packets_out_ctx,
	const struct lsquic_out_spec* out_spec,
	unsigned n_packets_out
)
{
	auto cxt = reinterpret_cast<Context*>(packets_out_ctx);
	std::vector<uv_buf_t> bufs;
	int succ_num = n_packets_out;
	for (int n = 0; n < n_packets_out; ++n)
	{
		if(bufs.size() < out_spec[n].iovlen)
			bufs.resize(out_spec[n].iovlen);
		for (int i = 0; i < out_spec[n].iovlen; ++i)
		{
			bufs[i].base = (char*)(out_spec[n].iov[i].iov_base);
			bufs[i].len = out_spec[n].iov[i].iov_len;
		}
		int ret = uv_udp_try_send( cxt->socket.get(), bufs.data(), bufs.size(),out_spec[n].dest_sa);
		if(ret < 0)
		{
			print_uv_err(ret,"try_send");
			--succ_num;
		}
	}
	return succ_num;
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
	while (true)
	{
		len = ::lsquic_stream_read(s, buf, sizeof(buf));
		if(len > 0)
			str.append(buf, len);
		else if(len == 0)
		{
			lsquic_stream_close(s);
			return;
		}else
			break;
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
	::lsquic_stream_wantread(s, 1);
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
