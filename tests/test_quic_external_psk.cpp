#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <netinet/in.h>
#endif
#include <lsquic.h>
#include <openssl/ssl.h>
#include <array>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <deque>
#include <thread>
#include <vector>

struct Node;
static bool retry_missing = false;
struct Packet { std::vector<unsigned char> bytes; Node* target; };
struct Node { SSL_CTX* tls{}; lsquic_engine_t* engine{}; lsquic_conn_t* conn{}; Node* other{}; sockaddr_in address{}; std::deque<Packet>* packets{}; bool is_server{}; bool connected{}; bool failed{}; bool retry_sent{}; };
static SSL_CTX* tls_context(void* peer, const sockaddr*) { return static_cast<Node*>(peer)->tls; }
static lsquic_conn_ctx_t* new_conn(void* ctx, lsquic_conn_t* conn) { auto* n=static_cast<Node*>(ctx); n->conn=conn; if(n->is_server) n->connected=true; return reinterpret_cast<lsquic_conn_ctx_t*>(n); }
static void conn_closed(lsquic_conn_t* conn) { auto* n=reinterpret_cast<Node*>(lsquic_conn_get_ctx(conn)); if(n) n->conn=nullptr; lsquic_conn_set_ctx(conn,nullptr); }
static lsquic_stream_ctx_t* new_stream(void*, lsquic_stream_t*) { return nullptr; }
static void stream_io(lsquic_stream_t*, lsquic_stream_ctx_t*) {}
static void handshake_done(lsquic_conn_t* c, lsquic_hsk_status status) { auto* n=reinterpret_cast<Node*>(lsquic_conn_get_ctx(c)); if(n) { n->connected=status==LSQ_HSK_OK || status==LSQ_HSK_RESUMED_OK; n->failed=!n->connected; } }
static int send_packets(void* ctx, const lsquic_out_spec* packets, unsigned count) {
    auto* n=static_cast<Node*>(ctx);
    for(unsigned i=0;i<count;++i) { Packet p; p.target=n->other; for(size_t j=0;j<packets[i].iovlen;++j) { const auto* b=static_cast<const unsigned char*>(packets[i].iov[j].iov_base); p.bytes.insert(p.bytes.end(),b,b+packets[i].iov[j].iov_len); }
        if(n->is_server && p.bytes.size() >= 5 && (p.bytes[0] & 0x80)) {
            const bool v1=p.bytes[1]==0 && p.bytes[2]==0 && p.bytes[3]==0 && p.bytes[4]==1;
            const bool v2=p.bytes[1]==0x6b && p.bytes[2]==0x33 && p.bytes[3]==0x43 && p.bytes[4]==0xcf;
            const auto type=(p.bytes[0] >> 4) & 3;
            n->retry_sent |= (v1 && type==3) || (v2 && type==0);
        }
        n->packets->push_back(std::move(p)); }
    return count;
}
static int choose_alpn(SSL*, const unsigned char** out, unsigned char* outlen, const unsigned char* in, unsigned int len, void*) {
    static const unsigned char name[]={4,'m','q','a','s'}; unsigned char* selected=nullptr;
    if(SSL_select_next_proto(&selected,outlen,name,sizeof(name),in,len)!=OPENSSL_NPN_NEGOTIATED) return SSL_TLSEXT_ERR_ALERT_FATAL;
    *out=selected; return SSL_TLSEXT_ERR_OK;
}
static bool run(bool bad, lsquic_version version, bool stepped = false) {
    std::deque<Packet> pending; Node client,server; server.is_server=true; client.other=&server; server.other=&client; client.packets=server.packets=&pending;
    client.address.sin_family=server.address.sin_family=AF_INET; client.address.sin_addr.s_addr=server.address.sin_addr.s_addr=htonl(INADDR_LOOPBACK);
    client.address.sin_port=htons(12345); server.address.sin_port=htons(12346);
    client.tls=SSL_CTX_new(TLS_method()); server.tls=SSL_CTX_new(TLS_method());
    std::array<unsigned char,32> key{1,2,3}; const unsigned char id[]={5,6}, context[]={7,8};
    SSL_CTX_set_min_proto_version(client.tls,TLS1_3_VERSION); SSL_CTX_set_max_proto_version(client.tls,TLS1_3_VERSION);
    SSL_CTX_set_min_proto_version(server.tls,TLS1_3_VERSION); SSL_CTX_set_max_proto_version(server.tls,TLS1_3_VERSION);
    SSL_CTX_set_alpn_select_cb(server.tls,choose_alpn,nullptr);
    auto* a=SSL_CREDENTIAL_new_pre_shared_key(key.data(),key.size(),id,sizeof id,EVP_sha256(),context,sizeof context); if(bad) key[0]^=1;
    auto* b=SSL_CREDENTIAL_new_pre_shared_key(key.data(),key.size(),id,sizeof id,EVP_sha256(),context,sizeof context);
    bool ok=a && b && SSL_CTX_add1_credential(client.tls,a) && SSL_CTX_add1_credential(server.tls,b);
    SSL_CREDENTIAL_free(a); SSL_CREDENTIAL_free(b);
    lsquic_stream_if si{}; si.on_new_conn=new_conn; si.on_conn_closed=conn_closed; si.on_new_stream=new_stream; si.on_read=stream_io; si.on_write=stream_io; si.on_close=stream_io; si.on_hsk_done=handshake_done;
    lsquic_engine_settings cs{}, ss{}; lsquic_engine_init_settings(&cs,0); lsquic_engine_init_settings(&ss,LSENG_SERVER);
    cs.es_proc_time_thresh=1000; ss.es_proc_time_thresh=1000;
    lsquic_engine_api ca{}; ca.ea_settings=&cs; ca.ea_stream_if=&si; ca.ea_stream_if_ctx=&client; ca.ea_packets_out=send_packets; ca.ea_packets_out_ctx=&client; ca.ea_get_ssl_ctx=tls_context; ca.ea_alpn="mqas";
    lsquic_engine_api sa=ca; sa.ea_settings=&ss; sa.ea_stream_if_ctx=&server; sa.ea_packets_out_ctx=&server;
    if(ok) { client.engine=lsquic_engine_new(0,&ca); server.engine=lsquic_engine_new(LSENG_SERVER,&sa); ok=client.engine && server.engine; }
    if(ok) ok=lsquic_engine_connect(client.engine,version,reinterpret_cast<sockaddr*>(&client.address),reinterpret_cast<sockaddr*>(&server.address),&client,reinterpret_cast<lsquic_conn_ctx_t*>(&client),nullptr,0,nullptr,0,nullptr,0)!=nullptr;
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(2);
    while(ok && std::chrono::steady_clock::now() < deadline && !client.failed && !(client.connected && server.connected)) {
        lsquic_engine_process_conns(client.engine); lsquic_engine_process_conns(server.engine);
        size_t n=pending.size(); while(n-- && !pending.empty()) { auto p=std::move(pending.front()); pending.pop_front(); auto* target=p.target; auto* other=target->other; lsquic_engine_packet_in(target->engine,p.bytes.data(),p.bytes.size(),reinterpret_cast<sockaddr*>(&target->address),reinterpret_cast<sockaddr*>(&other->address),target,0); if(stepped) lsquic_engine_process_conns(target->engine); }
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    bool success=ok && client.connected && server.connected;
#ifndef NDEBUG
    if (const char* force = std::getenv("LSQUIC_FORCE_RETRY"); force && std::strcmp(force,"1")==0 && !server.retry_sent) {
        std::fprintf(stderr,"Expected a QUIC Retry packet but none was sent\n");
        retry_missing=true;
    }
#endif
    if(client.conn) lsquic_conn_close(client.conn);
    if(server.conn) lsquic_conn_close(server.conn);
    for(int i=0;i<100 && (client.conn || server.conn);++i) {
        lsquic_engine_process_conns(client.engine); lsquic_engine_process_conns(server.engine);
        size_t n=pending.size(); while(n-- && !pending.empty()) { auto p=std::move(pending.front()); pending.pop_front(); auto* target=p.target; auto* other=target->other; lsquic_engine_packet_in(target->engine,p.bytes.data(),p.bytes.size(),reinterpret_cast<sockaddr*>(&target->address),reinterpret_cast<sockaddr*>(&other->address),target,0); if(stepped) lsquic_engine_process_conns(target->engine); }
    }
    if(client.engine) lsquic_engine_destroy(client.engine); if(server.engine) lsquic_engine_destroy(server.engine);
    SSL_CTX_free(client.tls); SSL_CTX_free(server.tls); return success && ok;
}
int main() { if(lsquic_global_init(LSQUIC_GLOBAL_CLIENT|LSQUIC_GLOBAL_SERVER)!=0) return 2; bool good=run(false,LSQVER_I001),bad=run(true,LSQVER_I001),v2good=run(false,LSQVER_I002),v2bad=run(true,LSQVER_I002),stepped=run(false,N_LSQVER,true),stepped_bad=run(true,N_LSQVER,true); lsquic_global_cleanup(); std::printf("QUIC external PSK: v1=%d wrong_v1=%d v2=%d wrong_v2=%d stepped=%d wrong_stepped=%d\n",good,bad,v2good,v2bad,stepped,stepped_bad); return good && !bad && v2good && !v2bad && stepped && !stepped_bad && !retry_missing ? 0 : 1; }
