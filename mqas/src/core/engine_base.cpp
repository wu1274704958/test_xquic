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

void mqas::core::IEngine::init(void* engine_base_ptr)
{
	engine_base_ptr_ = engine_base_ptr;
}

void mqas::core::IEngine::on_new_lsquic_engine(lsquic_engine_api&, EngineFlags){}

void mqas::core::IEngine::on_init_socket(io::UdpSocket*){}

void mqas::core::IEngine::on_init_logger(){}

bool mqas::core::IEngine::on_recv(const std::optional<std::span<char>>& buf, ssize_t nread, const sockaddr* addr,unsigned flags)
{
	LOG(INFO) << "on_recv " << nread << " bytes";
	return true;
}

void mqas::core::IEngine::on_new_conn(void* stream_if_ctx, lsquic_conn_t* lsquic_conn)
{
	LOG(INFO) << "on_new_conn " << reinterpret_cast<size_t>(lsquic_conn);
}

void mqas::core::IEngine::on_conn_closed(lsquic_conn_t* lsquic_conn)
{
	LOG(INFO) << "on_conn_closed " << reinterpret_cast<size_t>(lsquic_conn);
}

void mqas::core::IEngine::on_new_stream(void* stream_if_ctx, lsquic_stream_t* lsquic_stream)
{
	LOG(INFO) << "on_new_stream " << reinterpret_cast<size_t>(lsquic_stream);
}

void mqas::core::IEngine::on_read(lsquic_stream_t* lsquic_stream, lsquic_stream_ctx_t* lsquic_stream_ctx)
{
	LOG(INFO) << "on_read " << reinterpret_cast<size_t>(lsquic_stream);
}

void mqas::core::IEngine::on_write(lsquic_stream_t* lsquic_stream, lsquic_stream_ctx_t* lsquic_stream_ctx)
{
	LOG(INFO) << "on_write " << reinterpret_cast<size_t>(lsquic_stream);
}

void mqas::core::IEngine::on_close(lsquic_stream_t* lsquic_stream, lsquic_stream_ctx_t* lsquic_stream_ctx)
{
	LOG(INFO) << "on_close " << reinterpret_cast<size_t>(lsquic_stream);
}
//Optional callback
void mqas::core::IEngine::on_goaway_received(lsquic_conn_t* c)
{
	LOG(INFO) << "on_close " << reinterpret_cast<size_t>(c);
}
ssize_t mqas::core::IEngine::on_dg_write(lsquic_conn_t* c, void* buf, size_t size)
{
	LOG(INFO) << "on_dg_write " << reinterpret_cast<size_t>(c) <<  " size = " << size;
	buf = const_cast<char*>("dg");
	return 2;
}
void mqas::core::IEngine::on_datagram(lsquic_conn_t* c, const void* buf, size_t size)
{
	LOG(INFO) << "on_datagram " << reinterpret_cast<size_t>(c) << " size = " << size;
}
void mqas::core::IEngine::on_hsk_done(lsquic_conn_t* c, enum lsquic_hsk_status s)
{
	LOG(INFO) << "on_hsk_done " << reinterpret_cast<size_t>(c) << " status = " << s;
}
void mqas::core::IEngine::on_new_token(lsquic_conn_t* c, const unsigned char* token, size_t token_size)
{
	LOG(INFO) << "on_new_token " << reinterpret_cast<size_t>(c) << " token = " << token;
}
void mqas::core::IEngine::on_reset(lsquic_stream_t* s, lsquic_stream_ctx_t* h, int how)
{
	LOG(INFO) << "on_reset " << reinterpret_cast<size_t>(s) << " how = " << how;
}
void mqas::core::IEngine::on_conncloseframe_received(lsquic_conn_t* c, int app_error, uint64_t error_code, const char* reason, int reason_len)
{
	LOG(INFO) << "on_conncloseframe_received " << reinterpret_cast<size_t>(c) << "err_code = " << error_code << " " << reason;
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




int ssl_select_alpn_s(SSL* ssl, const unsigned char** out, unsigned char* outlen,
	const unsigned char* in, unsigned inlen, void* arg)
{
	const auto alpn = static_cast<const char*>(arg);
	LOG(INFO) << "select alpn";
	std::vector<char> buf;
	const auto ss = mqas::comm::split(alpn, ';');
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
		const std::string_view in_sv(reinterpret_cast<const char*>(in), inlen);
		LOG(TRACE) << "no supported protocol can be selected from " << in_sv;
		return SSL_TLSEXT_ERR_ALERT_FATAL;
	}
}

///read lsquic setting from config
#define CK_READ_SETTING(k,t) if (v.contains(#k)) s.k = toml::find<t>(v, #k)
#define CK_READ_SETTING_Str(k) if (v.contains(#k)) s.k = toml::find<std::string>(v, #k).c_str()
void settings_from_toml(::lsquic_engine_settings& s, const toml::value& v)
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
