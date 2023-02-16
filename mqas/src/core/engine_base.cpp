#include <fstream>
#include<mqas/core/engine_base.h>
#include <sstream>
#include <stdexcept>
#include <mqas/io/udp.h>
#include <mqas/io/timer.h>
#include "easylogging++.h"
#include <lsquic.h>
#include <mqas/log.h>
#include <mqas/comm/macro.h>
#include <mqas/io/ip.h>
#include <openssl/pem.h>
#include <mqas/comm/string.h>

#ifdef PF_ANDROID
#endif
mqas::core::engine_base::engine_base(io::Context& c):cxt(c),socket_(nullptr)
,proc_conns_timer_(nullptr)
{}

mqas::core::engine_base::engine_base(engine_base&& oth) noexcept : cxt(oth.cxt)
{
	socket_ = oth.socket_;
	oth.socket_ = nullptr;

	proc_conns_timer_ = oth.proc_conns_timer_;
	oth.proc_conns_timer_ = nullptr;
}

void mqas::core::engine_base::init(const char* conf_file,core::EngineFlags engine_flags)
{
	//parse config 
	const auto conf_data = toml::parse(conf_file);
	conf_ = std::make_shared<engine_config>(toml::find<engine_config>(conf_data, "engine_config"));
	
	::lsquic_engine_init_settings(&conf_->lsquic_settings, static_cast<unsigned>(engine_flags));
	if(conf_data.contains("lsquic_settings"))
		settings_from_toml(conf_->lsquic_settings,conf_data.at("lsquic_settings"));
	//init socket
	socket_ = cxt.make_handle<io::UdpSocket>();
	sockaddr addr{};
	io::Ip::str2addr_ipv4(conf_->bind_ip.c_str(),conf_->port, addr);
	socket_->bind(addr,UV_UDP_REUSEADDR);
	// init timer
	proc_conns_timer_ = cxt.make_handle<io::Timer>();
	//init logger
	init_logger();
	//init ssl
	if (contain<uint32_t>(engine_flags, EngineFlags::Server) && !conf_->ssl_cert_path.empty() && !conf_->ssl_key_path.empty())
		init_ssl(conf_->ssl_cert_path.c_str(),conf_->ssl_key_path.c_str());
	init_lsquic(engine_flags);
}

void mqas::core::engine_base::init_logger() const
{
	mqas::log::init("default",conf_->log_config,std::nullopt);
	const auto lsquic_log = el::Loggers::getLogger("lsquic");
	el::Configurations c;
	c.setFromBase(el::Loggers::getLogger("default")->configurations());
	auto fmt = c.get(el::Level::Global,el::ConfigurationType::Format)->value();
	auto p = fmt.find("[%level]");
	if(p != std::string::npos){
		fmt.replace(p, sizeof("[%level]"), "");
		c.set(el::Level::Global,el::ConfigurationType::Format,fmt);
	}
	el::Loggers::reconfigureLogger(lsquic_log, c);
}

void mqas::core::engine_base::init_lsquic(core::EngineFlags flags) noexcept(false)
{
	constexpr ::lsquic_logger_if logger_if = { lsquic_log_func, };

	lsquic_logger_init(&logger_if, this, LLTS_NONE);
	lsquic_set_log_level(conf_->log_level.c_str());

	::lsquic_stream_if stream_if = {};
	stream_if.on_new_conn = on_new_conn_s;
	stream_if.on_conn_closed = on_conn_closed_s;
	stream_if.on_new_stream = on_new_stream_s;
	stream_if.on_read = on_read_s;
	stream_if.on_write = on_write_s;
	stream_if.on_close = on_close_s;

	::lsquic_engine_api engine_api = {};
	engine_api.ea_settings = &conf_->lsquic_settings;
	engine_api.ea_packets_out = on_packets_out;
	engine_api.ea_packets_out_ctx = this;
	engine_api.ea_stream_if = &stream_if;
	engine_api.ea_stream_if_ctx = this;
	if(contain<uint32_t>(flags,EngineFlags::Server))
		engine_api.ea_get_ssl_ctx = on_get_ssl_ctx;
	if(flags == EngineFlags::None)
	{
		engine_api.ea_alpn = conf_->alpn.c_str();
		if(conf_->alpn.empty())
			LOG(WARNING) << "Client alpn is empty!";
	}
	engine_ = lsquic_engine_new(static_cast<unsigned>(flags), &engine_api);
	if(engine_ == nullptr)
	{
		LOG(ERROR) << "Create engine failed!";
		throw std::runtime_error("Create engine failed!");
	}
	LOG(INFO) << "Create engine success!";
}

void mqas::core::engine_base::close_socket()
{
	if(socket_)
	{
		cxt.del_handle(socket_);
		socket_ = nullptr;
	}
}

void mqas::core::engine_base::close_timer()
{
	if (proc_conns_timer_)
	{
		cxt.del_handle(proc_conns_timer_);
		proc_conns_timer_ = nullptr;
	}
}

void mqas::core::engine_base::close_ssl_ctx()
{
	if (ssl_ctx_)
	{
		SSL_CTX_free(ssl_ctx_);
		ssl_ctx_ = nullptr;
	}
}

mqas::core::engine_base::~engine_base()
{
	close_socket();
	close_timer();
	close_ssl_ctx();
}

std::string mqas::core::engine_base::load_config(const char* conf_file)
{
	std::stringstream ss;
#ifdef PF_ANDROID

#else
	std::ifstream file(conf_file);
	if (!file.is_open())
		throw std::runtime_error("Load config file failed!");
	ss << file.rdbuf();
	file.close();
#endif
	return ss.str();
}
//engine config deserialize
mqas::core::engine_config toml::from<mqas::core::engine_config>::from_toml(const value& v)
{
	mqas::core::engine_config f;
	f.bind_ip =		find_or<std::string		>(v,		"bind_ip",		"0.0.0.0");
	f.port =		find_or<short			>(v,		"port",			8083);
	f.log_level =	find_or<std::string		>(v,		"log_level",	"warning");
	f.log_path=		find_or<std::string		>(v,		"log_path",		"log.txt");
	f.log_config =	find_or<std::string		>(v,		"log_config",	"");
	f.alpn =		find_or<std::string		>(v,		"alpn",			"");
	f.ssl_cert_path=find_or<std::string		>(v,		"ssl_cert_path","");
	f.ssl_key_path =find_or<std::string		>(v,		"ssl_key_path", "");
	return f;
}

//lsquic callback function implement
int mqas::core::engine_base::lsquic_log_func(void* logger_ctx, const char* buf, size_t len)
{
	CLOG(ERROR, "lsquic") << buf;
	return 0;
}

lsquic_conn_ctx_t* mqas::core::engine_base::on_new_conn_s(void* stream_if_ctx, lsquic_conn_t* lsquic_conn)
{
	return nullptr;
}

void mqas::core::engine_base::on_conn_closed_s(lsquic_conn_t* lsquic_conn)
{
}

lsquic_stream_ctx_t* mqas::core::engine_base::on_new_stream_s(void* stream_if_ctx, lsquic_stream_t* lsquic_stream)
{
	return nullptr;
}

void mqas::core::engine_base::on_read_s(lsquic_stream_t* lsquic_stream, lsquic_stream_ctx_t* lsquic_stream_ctx)
{
}

void mqas::core::engine_base::on_write_s(lsquic_stream_t* lsquic_stream, lsquic_stream_ctx_t* lsquic_stream_ctx)
{
}

void mqas::core::engine_base::on_close_s(lsquic_stream_t* lsquic_stream, lsquic_stream_ctx_t* lsquic_stream_ctx)
{
}

int mqas::core::engine_base::on_packets_out(void* packets_out_ctx, const lsquic_out_spec* out_spec,
	unsigned n_packets_out)
{
	return 0;
}

ssl_ctx_st* mqas::core::engine_base::on_get_ssl_ctx(void* peer_ctx, const sockaddr* local)
{
	const auto engine = static_cast<engine_base*>(peer_ctx);
	return engine->ssl_ctx_;
}

int mqas::core::engine_base::ssl_select_alpn(SSL* ssl, const unsigned char** out, unsigned char* outlen,
	const unsigned char* in, unsigned inlen) const
{
	LOG(INFO) << "select alpn";
	std::vector<char> buf;
	const auto ss = comm::split(conf_->alpn,';');
	for (auto& a : ss)
	{
		buf.push_back(static_cast<char>(a.size()));
		for (auto c : a)
			buf.push_back(c);
	}
	const int r = SSL_select_next_proto(const_cast<unsigned char**>(out), outlen, in, inlen,
	reinterpret_cast<const uint8_t*>(buf.data()), static_cast<unsigned>(buf.size()));
	if (r == OPENSSL_NPN_NEGOTIATED)
		return SSL_TLSEXT_ERR_OK;
	else {
		const std::string_view in_sv(reinterpret_cast<const char*>(in),inlen);
		LOG(TRACE) << "no supported protocol can be selected from " << in_sv;
		return SSL_TLSEXT_ERR_ALERT_FATAL;
	}
}

int ssl_select_alpn_s(SSL* ssl, const unsigned char** out, unsigned char* outlen,
	const unsigned char* in, unsigned inlen, void* arg)
{
	const auto engine = static_cast<mqas::core::engine_base*>(arg);
	return engine->ssl_select_alpn(ssl,out,outlen,in,inlen);
}

int mqas::core::engine_base::init_ssl(const char* cert_file, const char* key_file)
{
	LOG(INFO) << "initialize ssl ctx";
	int ret = 0;
	ssl_ctx_ = SSL_CTX_new(TLS_method());
	if (!ssl_ctx_)
	{
		LOG(ERROR) << "SSL_CTX_new failed";
		throw std::runtime_error("SSL_CTX_new failed\n");
	}
	SSL_CTX_set_min_proto_version(ssl_ctx_, TLS1_3_VERSION);
	SSL_CTX_set_max_proto_version(ssl_ctx_, TLS1_3_VERSION);
	SSL_CTX_set_default_verify_paths(ssl_ctx_);
	SSL_CTX_set_alpn_select_cb(ssl_ctx_, ssl_select_alpn_s, this);
	if ((ret = SSL_CTX_use_certificate_chain_file(ssl_ctx_, cert_file)) != 1)
	{
		LOG(ERROR) << "SSL_CTX_use_certificate_chain_file failed " << ret;
		throw std::runtime_error("SSL_CTX_use_certificate_chain_file failed");
	}
	if ((ret = SSL_CTX_use_PrivateKey_file(ssl_ctx_, key_file, SSL_FILETYPE_PEM)) != 1)
	{
		LOG(ERROR) << "SSL_CTX_use_PrivateKey_file failed " << ret;
		throw std::runtime_error("SSL_CTX_use_PrivateKey_file failed");
	}
	return ret;
}

///read lsquic setting from config
#define CK_READ_SETTING(k,t) if (v.contains(#k)) s.k = toml::find<t>(v, #k)
#define CK_READ_SETTING_Str(k) if (v.contains(#k)) s.k = toml::find<std::string>(v, #k).c_str()
void mqas::core::engine_base::settings_from_toml(::lsquic_engine_settings& s, const toml::value& v)
{
	CK_READ_SETTING(es_versions,unsigned);
	CK_READ_SETTING(es_sfcw, unsigned);
	CK_READ_SETTING(es_max_cfcw, unsigned);
	CK_READ_SETTING(es_max_sfcw, unsigned);
	CK_READ_SETTING(es_max_streams_in, unsigned);
	CK_READ_SETTING(es_handshake_to, unsigned long);
	CK_READ_SETTING(es_idle_conn_to, unsigned long);
	CK_READ_SETTING(es_silent_close, int);
	CK_READ_SETTING(es_max_header_list_size, unsigned);
	CK_READ_SETTING(es_silent_close, int);
	CK_READ_SETTING_Str(es_ua);
	CK_READ_SETTING(es_sttl, uint64_t);
	CK_READ_SETTING(es_pdmd, uint64_t);
	CK_READ_SETTING(es_aead, uint64_t);
	CK_READ_SETTING(es_kexs, uint64_t);
	CK_READ_SETTING(es_max_inchoate, unsigned);
	CK_READ_SETTING(es_support_push, unsigned);
	CK_READ_SETTING(es_support_tcid0, int);
	CK_READ_SETTING(es_support_nstp, int);
	CK_READ_SETTING(es_honor_prst, int);
	CK_READ_SETTING(es_send_prst, int);
	CK_READ_SETTING(es_progress_check, unsigned);
	CK_READ_SETTING(es_rw_once, int);
	CK_READ_SETTING(es_proc_time_thresh, unsigned);
	CK_READ_SETTING(es_pace_packets, int);
	CK_READ_SETTING(es_clock_granularity, unsigned);
	CK_READ_SETTING(es_cc_algo, unsigned);
	CK_READ_SETTING(es_cc_rtt_thresh, unsigned);
	CK_READ_SETTING(es_noprogress_timeout, unsigned);
	CK_READ_SETTING(es_init_max_data, unsigned);
	CK_READ_SETTING(es_init_max_stream_data_bidi_remote, unsigned);
	CK_READ_SETTING(es_init_max_stream_data_bidi_local, unsigned);
	CK_READ_SETTING(es_init_max_stream_data_uni, unsigned);
	CK_READ_SETTING(es_init_max_streams_bidi, unsigned);
	CK_READ_SETTING(es_init_max_streams_uni, unsigned);
	CK_READ_SETTING(es_idle_timeout, unsigned);
	CK_READ_SETTING(es_ping_period, unsigned);
	CK_READ_SETTING(es_scid_len, unsigned);
	CK_READ_SETTING(es_scid_iss_rate, unsigned);
	CK_READ_SETTING(es_qpack_dec_max_size, unsigned);
	CK_READ_SETTING(es_qpack_dec_max_blocked, unsigned);
	CK_READ_SETTING(es_qpack_enc_max_size, unsigned);
	CK_READ_SETTING(es_qpack_enc_max_blocked, unsigned);
	CK_READ_SETTING(es_ecn, int);
	CK_READ_SETTING(es_allow_migration, int);
	CK_READ_SETTING(es_ql_bits, int);
	CK_READ_SETTING(es_spin, int);
	CK_READ_SETTING(es_delayed_acks, int);
	CK_READ_SETTING(es_timestamps, int);
	CK_READ_SETTING(es_max_udp_payload_size_rx, int);
	CK_READ_SETTING(es_grease_quic_bit, int);
	CK_READ_SETTING(es_dplpmtud, int);
	CK_READ_SETTING(es_base_plpmtu, unsigned short);
	CK_READ_SETTING(es_max_plpmtu, unsigned short);
	CK_READ_SETTING(es_mtu_probe_timer, unsigned);
	CK_READ_SETTING(es_datagrams, int);
	CK_READ_SETTING(es_optimistic_nat, int);
	CK_READ_SETTING(es_ext_http_prio, int);
	CK_READ_SETTING(es_qpack_experiment, int);
	/**
	*WARNING.The library comes with sane defaults.Only fiddle with
	* these knobs if you know what you are doing.
	*/
	CK_READ_SETTING(es_ptpc_periodicity, unsigned);
	CK_READ_SETTING(es_ptpc_max_packtol, unsigned);
	CK_READ_SETTING(es_ptpc_dyn_target, int);
	CK_READ_SETTING(es_ptpc_target, float);
	CK_READ_SETTING(es_ptpc_prop_gain, float);
	CK_READ_SETTING(es_ptpc_int_gain, float);
	CK_READ_SETTING(es_ptpc_err_thresh, float);
	CK_READ_SETTING(es_ptpc_err_divisor, float);

	CK_READ_SETTING(es_delay_onclose, int);
	CK_READ_SETTING(es_max_batch_size, unsigned);
	CK_READ_SETTING(es_check_tp_sanity, int);
}
#undef CK_READ_SETTING
#undef CK_READ_SETTING_Str
