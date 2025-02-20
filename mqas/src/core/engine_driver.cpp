#include "mqas/core/engine_driver.h"
#include "mqas/comm/string.h"
#include <easylogging++.h>
#include "mqas/log.h"
#include <mqas/comm/macro.h>
#include <mqas/io/udp.h>
#include <mqas/io/exception.h>
#include <mqas/io/ip.h>

namespace mqas::core {

	std::shared_ptr<engine_driver> engine_driver::_instance = nullptr;
	std::mutex engine_driver::_instance_lock;
	std::shared_ptr<engine_driver> engine_driver::instance() {
		std::lock_guard lock(_instance_lock);
		if (_instance == nullptr)
			_instance = std::make_shared<engine_driver>();
		return _instance;
	}

	bool engine_driver::register_engine(engine_base_interface* e, ::lsquic_engine* origin_e)
	{
		if(!_initialized)
			return false;

		MQAS_DBG("register_engine " << e << " flags = " << (int)e->get_engine_flags());

		auto v = _engine_map.find(origin_e);
		if(v != _engine_map.end())
			return false;
		_engine_map.insert({ origin_e, e});
		return true;
	}

	void engine_driver::unregister_engine(engine_base_interface* e, ::lsquic_engine* origin_e)
	{
		_engine_map.erase(origin_e);
	}

	bool engine_driver::initialization(const char* conf)
	{
		if(initialized())
			return true;
		_golbal_conf = std::make_shared<toml::value>(toml::parse(conf));
		_engine_config = std::make_shared<engine_config>(toml::find<engine_config>(*_golbal_conf, "engine_config"));

		init_logger();

		init_lsquic();
		_initialized = true;
		return true;
	}

	/*void engine_driver::init_setting(const toml::value& conf_data, EngineFlags flags)
	{
		::lsquic_engine_init_settings(&_engine_config->lsquic_settings, static_cast<unsigned>(flags));
		if (conf_data.contains("lsquic_settings"))
			settings_from_toml(_engine_config->lsquic_settings, conf_data.at("lsquic_settings"));
	}*/

	void engine_driver::fill_lsquic_engine_api(engine_base_interface* e,EngineFlags flags,::lsquic_engine_api& api,
		const engine_config& conf) const
	{
		api.ea_packets_out = on_packets_out;
		api.ea_stream_if = &_lsquic_stream_if;
		api.ea_stream_if_ctx = (void*)e;
	}

	//
	void engine_driver::init_logger() const
	{
		mqas::log::init("default", _engine_config->log_config, std::nullopt);
		const auto lsquic_log = el::Loggers::getLogger("lsquic");
		el::Configurations c;
		c.setFromBase(el::Loggers::getLogger("default")->configurations());
		auto fmt = c.get(el::Level::Global, el::ConfigurationType::Format)->value();
		bool erase_succ = mqas::comm::erase_substr(fmt, "[%level]");
		if (!erase_succ) erase_succ = mqas::comm::erase_substr(fmt, "[%levshort]");
		if (erase_succ)
			c.set(el::Level::Global, el::ConfigurationType::Format, fmt);
		el::Loggers::reconfigureLogger(lsquic_log, c);
	}

	::SSL_CTX* engine_driver::init_ssl(const std::string& cert_file, const std::string& key_file, const std::string& alpn)
	{
		//LOG(INFO) << "initialize ssl ctx";
		int ret = 0;
		auto ssl_ctx = SSL_CTX_new(TLS_method());
		if (!ssl_ctx)
		{
			LOG(ERROR) << "SSL_CTX_new failed";
			return nullptr;
		}
		SSL_CTX_set_min_proto_version(ssl_ctx, TLS1_3_VERSION);
		SSL_CTX_set_max_proto_version(ssl_ctx, TLS1_3_VERSION);
		SSL_CTX_set_default_verify_paths(ssl_ctx);
		SSL_CTX_set_alpn_select_cb(ssl_ctx, ssl_select_alpn_s, const_cast<char*>(alpn.c_str()));
		if ((ret = SSL_CTX_use_certificate_chain_file(ssl_ctx, cert_file.c_str())) != 1)
		{
			LOG(ERROR) << "SSL_CTX_use_certificate_chain_file failed " << ret;
			SSL_CTX_free(ssl_ctx);
			return nullptr;
		}
		if ((ret = SSL_CTX_use_PrivateKey_file(ssl_ctx, key_file.c_str(), SSL_FILETYPE_PEM)) != 1)
		{
			LOG(ERROR) << "SSL_CTX_use_PrivateKey_file failed " << ret;
			SSL_CTX_free(ssl_ctx);
			return nullptr;
		}
		return ssl_ctx;
	}

	std::string engine_driver::ssl_pair_key(const std::string& cert_file, const std::string& key_file)
	{
		std::string res(cert_file);
		res += '_';
		res += key_file;
		return res;
	}

	::SSL_CTX* engine_driver::get_ssl_or_generate(const std::string& cert_file, const std::string& key_file,const std::string& alpn)
	{
		const auto key = ssl_pair_key(cert_file,key_file);
		if(_ssl_ctx_map.contains(key))
			return _ssl_ctx_map[key];
		auto res = init_ssl(cert_file,key_file,alpn);
		if(res == nullptr)
			return res;
		_ssl_ctx_map.insert({ std::move(key),res});
		return res;
	}

	::SSL_CTX* engine_driver::destroy_ssl_ctx(::SSL_CTX* ctx)
	{
		if(ctx == nullptr)
			return nullptr;
		for (auto it = _ssl_ctx_map.begin(); it != _ssl_ctx_map.end(); ++it)
		{
			if (it->second == ctx)
			{
				SSL_CTX_free(it->second);
				_ssl_ctx_map.erase(it);
				return nullptr;
			}
		}
		return nullptr;
	}

	void engine_driver::close_ssl_ctx()
	{
		for (auto it = _ssl_ctx_map.begin();it != _ssl_ctx_map.end();++it)
		{
			SSL_CTX_free(it->second);
		}
		_ssl_ctx_map.clear();
	}

	void engine_driver::init_lsquic() noexcept(false)
	{
		_lsquic_logger_if = { lsquic_log_func, };

		lsquic_logger_init(&_lsquic_logger_if, this, LLTS_NONE);
		lsquic_set_log_level(_engine_config->log_level.c_str());

		_lsquic_stream_if.on_new_conn = on_new_conn_s;
		_lsquic_stream_if.on_conn_closed = on_conn_closed_s;
		_lsquic_stream_if.on_new_stream = on_new_stream_s;
		_lsquic_stream_if.on_read = on_read_s;
		_lsquic_stream_if.on_write = on_write_s;
		_lsquic_stream_if.on_close = on_close_s;
		_lsquic_stream_if.on_goaway_received = on_goaway_received;
		_lsquic_stream_if.on_dg_write = on_dg_write;
		_lsquic_stream_if.on_datagram = on_datagram;
		_lsquic_stream_if.on_hsk_done = on_hsk_done;
		_lsquic_stream_if.on_new_token = on_new_token;
		_lsquic_stream_if.on_reset = on_reset;
		_lsquic_stream_if.on_conncloseframe_received = on_conncloseframe_received;
	}


	//lsquic callback function
	int engine_driver::lsquic_log_func(void* logger_ctx, const char* buf, size_t len)
	{
		CLOG(ERROR, "lsquic") << buf;
		return 0;
	}

	engine_base_interface* engine_driver::get_engine_by_cxt(void* cxt)
	{
		return static_cast<engine_base_interface*>(cxt);
	}
		

	lsquic_conn_ctx_t* engine_driver::on_new_conn_s(void* stream_if_ctx, lsquic_conn_t* lsquic_conn)
	{
		auto ptr = get_engine_by_cxt(stream_if_ctx);
		ptr->on_new_conn_s(stream_if_ctx,lsquic_conn);
		return reinterpret_cast<lsquic_conn_ctx_t*>(stream_if_ctx);
	}
	void engine_driver::on_conn_closed_s(lsquic_conn_t* lsquic_conn)
	{
		auto ptr = get_engine_by_cxt(::lsquic_conn_get_ctx(lsquic_conn));
		ptr->on_conn_closed_s(lsquic_conn);
	}
	lsquic_stream_ctx_t* engine_driver::on_new_stream_s(void* stream_if_ctx, lsquic_stream_t* lsquic_stream)
	{
		auto ptr = get_engine_by_cxt(stream_if_ctx);
		ptr->on_new_stream_s(stream_if_ctx, lsquic_stream);
		return reinterpret_cast<lsquic_stream_ctx_t*>(stream_if_ctx);
	}
	void engine_driver::on_read_s(lsquic_stream_t* lsquic_stream, lsquic_stream_ctx_t* lsquic_stream_ctx)
	{
		auto ptr = get_engine_by_cxt(lsquic_stream_ctx);
		ptr->on_read_s(lsquic_stream, lsquic_stream_ctx);
	}
	void engine_driver::on_write_s(lsquic_stream_t* lsquic_stream, lsquic_stream_ctx_t* lsquic_stream_ctx)
	{
		auto ptr = get_engine_by_cxt(lsquic_stream_ctx);
		MQAS_DBG("driver on_write_s stream = " << lsquic_stream << " cxt = " << ptr);
		ptr->on_write_s(lsquic_stream, lsquic_stream_ctx);
	}
	void engine_driver::on_close_s(lsquic_stream_t* lsquic_stream, lsquic_stream_ctx_t* lsquic_stream_ctx)
	{
		auto ptr = get_engine_by_cxt(lsquic_stream_ctx);
		ptr->on_close_s(lsquic_stream, lsquic_stream_ctx);
	}
	//optional callback
	void engine_driver::on_goaway_received(lsquic_conn_t* c)
	{
		auto ptr = get_engine_by_cxt(::lsquic_conn_get_ctx(c));
		ptr->on_goaway_received(c);
	}

	ssize_t engine_driver::on_dg_write(lsquic_conn_t* c, void* buf, size_t buf_sz)
	{
		auto ptr = get_engine_by_cxt(::lsquic_conn_get_ctx(c));
		return ptr->on_dg_write(c, buf, buf_sz);
	}

	void engine_driver::on_datagram(lsquic_conn_t* c, const void* buf, size_t sz)
	{
		auto ptr = get_engine_by_cxt(::lsquic_conn_get_ctx(c));
		ptr->on_datagram(c,buf,sz);
	}

	void engine_driver::on_hsk_done(lsquic_conn_t* c, enum lsquic_hsk_status s)
	{
		auto ptr = get_engine_by_cxt(::lsquic_conn_get_ctx(c));
		ptr->on_hsk_done(c, s);
	}
	void engine_driver::on_new_token(lsquic_conn_t* c, const unsigned char* token, size_t token_size)
	{
		auto ptr = get_engine_by_cxt(::lsquic_conn_get_ctx(c));
		ptr->on_new_token(c, token,token_size);
	}
	void engine_driver::on_reset(lsquic_stream_t* s, lsquic_stream_ctx_t* h, int how)
	{
		auto ptr = get_engine_by_cxt(h);
		ptr->on_reset(s, h, how);
	}

	void engine_driver::on_conncloseframe_received(lsquic_conn_t* c, int app_error, uint64_t error_code, const char* reason, int reason_len)
	{
		auto ptr = get_engine_by_cxt(::lsquic_conn_get_ctx(c));
		ptr->on_conncloseframe_received(c, app_error, error_code,reason,reason_len);
	}

	int engine_driver::on_packets_out(void* packets_out_ctx, const lsquic_out_spec* out_spec, unsigned n_packets_out)
	{
		const auto sock = static_cast<io::UdpSocket*>(packets_out_ctx);

		std::vector<std::span<uint8_t>> bufs;
		unsigned succ_num = n_packets_out;
		for (unsigned n = 0; n < n_packets_out; ++n)
		{
			if (bufs.size() < out_spec[n].iovlen)
				bufs.resize(out_spec[n].iovlen);
			for (unsigned i = 0; i < out_spec[n].iovlen; ++i)
			{
				bufs[i] = std::span<uint8_t>(static_cast<uint8_t*>(out_spec[n].iov[i].iov_base), out_spec[n].iov[i].iov_len);
			}
			try {
				sock->send(bufs, *out_spec[n].dest_sa, [](io::UdpSocket* s, int status) {
					if (status != 0) LOG(ERROR) << "packets_out send failed status = " << status;
					});
				//sock->try_send(bufs, *out_spec[n].dest_sa);
			}
			catch (io::Exception& e)
			{
				--succ_num;
				LOG(ERROR) << "packets_out send failed exception = " << e.what();
			}
		}
		return static_cast<int>(succ_num);
	}

}

namespace mqas::core {


	int engine_driver::ssl_select_alpn_s(::SSL* ssl, const unsigned char** out, unsigned char* outlen,
		const unsigned char* in, unsigned inlen, void* arg)
	{
		const auto alpn = static_cast<const char*>(arg);
		//LOG(INFO) << "select alpn";
		std::vector<uint8_t> buf;
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
#define CK_READ_SETTING(k,t,def) s.k = toml::find_or<t>(v, #k, def)
#define CK_READ_SETTING_Str(k,def) if (v.contains(#k)) s.k = toml::find<std::string>(v, #k).c_str(); else s.k = #def
	void engine_driver::settings_from_toml(::lsquic_engine_settings& s, const toml::value& v,bool is_server)
	{
CK_READ_SETTING(es_versions, unsigned,55);
CK_READ_SETTING(es_sfcw, unsigned,is_server ? LSQUIC_DF_SFCW_SERVER : LSQUIC_DF_SFCW_CLIENT);
CK_READ_SETTING(es_cfcw, unsigned,is_server ? LSQUIC_DF_CFCW_SERVER : LSQUIC_DF_CFCW_CLIENT);
CK_READ_SETTING(es_max_cfcw, unsigned, is_server ? LSQUIC_DF_INIT_MAX_DATA_SERVER : LSQUIC_DF_INIT_MAX_DATA_CLIENT);
CK_READ_SETTING(es_max_sfcw, unsigned,is_server ? LSQUIC_DF_INIT_MAX_STREAM_DATA_BIDI_REMOTE_SERVER : LSQUIC_DF_INIT_MAX_STREAM_DATA_BIDI_REMOTE_CLIENT);
CK_READ_SETTING(es_max_streams_in, unsigned,is_server ? LSQUIC_DF_INIT_MAX_STREAM_DATA_BIDI_REMOTE_SERVER : LSQUIC_DF_INIT_MAX_STREAM_DATA_BIDI_REMOTE_CLIENT);
CK_READ_SETTING(es_handshake_to, unsigned long, LSQUIC_DF_HANDSHAKE_TO); // Handshake timeout in milliseconds
CK_READ_SETTING(es_idle_conn_to, unsigned long, LSQUIC_DF_IDLE_CONN_TO); // Idle connection timeout in milliseconds
CK_READ_SETTING(es_silent_close, int, LSQUIC_DF_SILENT_CLOSE); // Default silent close behavior (0 = false)
CK_READ_SETTING(es_max_header_list_size, unsigned, LSQUIC_DF_MAX_HEADER_LIST_SIZE); // Maximum header list size

CK_READ_SETTING_Str(es_ua, LSQUIC_DF_UA); // User agent string for QUIC, default can be a generic value
CK_READ_SETTING(es_sttl, uint64_t, LSQUIC_DF_STTL); // Default server TTL (1 hour)
CK_READ_SETTING(es_pdmd, uint64_t, 5000); // Peer data message delay (5 seconds)
CK_READ_SETTING(es_aead, uint64_t, 0); // Default AEAD algorithm (no specific algorithm set)
CK_READ_SETTING(es_kexs, uint64_t, 0); // Default key exchange algorithm
CK_READ_SETTING(es_max_inchoate, unsigned, LSQUIC_DF_MAX_INCHOATE); // Max number of inchoate streams
CK_READ_SETTING(es_support_push, unsigned, LSQUIC_DF_SUPPORT_PUSH); // Support for server push (1 = true)
CK_READ_SETTING(es_support_tcid0, int, LSQUIC_DF_SUPPORT_TCID0); // Support for TCID 0 (1 = true)
CK_READ_SETTING(es_support_nstp, int, LSQUIC_DF_SUPPORT_NSTP); // Support for NSTP (0 = false)
CK_READ_SETTING(es_honor_prst, int, LSQUIC_DF_HONOR_PRST); // Honor prst (1 = true)
CK_READ_SETTING(es_send_prst, int, LSQUIC_DF_SEND_PRST); // Send prst (0 = false)
CK_READ_SETTING(es_progress_check, unsigned, LSQUIC_DF_PROGRESS_CHECK); // Progress check enabled
CK_READ_SETTING(es_rw_once, int, LSQUIC_DF_RW_ONCE); // Only read/write once
CK_READ_SETTING(es_proc_time_thresh, unsigned, LSQUIC_DF_PROC_TIME_THRESH); // Process time threshold in milliseconds
CK_READ_SETTING(es_pace_packets, int, LSQUIC_DF_PACE_PACKETS); // Pace packets (0 = no pacing)
CK_READ_SETTING(es_clock_granularity, unsigned, LSQUIC_DF_CLOCK_GRANULARITY); // Granularity of clock
CK_READ_SETTING(es_cc_algo, unsigned, LSQUIC_DF_CC_ALGO); // Default congestion control algorithm (2 could refer to CUBIC)
CK_READ_SETTING(es_cc_rtt_thresh, unsigned, LSQUIC_DF_CC_RTT_THRESH); // RTT threshold for congestion control (200 ms)
CK_READ_SETTING(es_noprogress_timeout, unsigned, is_server ? LSQUIC_DF_NOPROGRESS_TIMEOUT_SERVER : LSQUIC_DF_NOPROGRESS_TIMEOUT_CLIENT); // No progress timeout in milliseconds
CK_READ_SETTING(es_init_max_data, unsigned, is_server ? LSQUIC_DF_INIT_MAX_DATA_SERVER:LSQUIC_DF_INIT_MAX_DATA_CLIENT); // Initial max data (1 MB)
CK_READ_SETTING(es_init_max_stream_data_bidi_remote, unsigned, is_server ? LSQUIC_DF_INIT_MAX_STREAM_DATA_BIDI_REMOTE_SERVER : LSQUIC_DF_INIT_MAX_STREAM_DATA_BIDI_REMOTE_CLIENT); // Max stream data for bidi remote
CK_READ_SETTING(es_init_max_stream_data_bidi_local, unsigned, LSQUIC_DF_INIT_MAX_STREAM_DATA_BIDI_LOCAL_CLIENT); // Max stream data for bidi local
CK_READ_SETTING(es_init_max_stream_data_uni, unsigned, is_server ? LSQUIC_DF_INIT_MAX_STREAM_DATA_UNI_SERVER : LSQUIC_DF_INIT_MAX_STREAM_DATA_UNI_CLIENT); // Max stream data for uni streams
CK_READ_SETTING(es_init_max_streams_bidi, unsigned, LSQUIC_DF_INIT_MAX_STREAMS_BIDI); // Max bidirectional streams
CK_READ_SETTING(es_init_max_streams_uni, unsigned, is_server ? LSQUIC_DF_INIT_MAX_STREAMS_UNI_SERVER : LSQUIC_DF_INIT_MAX_STREAMS_UNI_CLIENT ); // Max unidirectional streams
CK_READ_SETTING(es_idle_timeout, unsigned, LSQUIC_DF_IDLE_TIMEOUT); // Idle timeout in milliseconds (5 minutes)
CK_READ_SETTING(es_ping_period, unsigned, LSQUIC_DF_PING_PERIOD); // Ping period in milliseconds (10 seconds)
CK_READ_SETTING(es_scid_len, unsigned, LSQUIC_DF_SCID_LEN); // SCID length (default 32 bytes)
CK_READ_SETTING(es_scid_iss_rate, unsigned, LSQUIC_DF_SCID_ISS_RATE); // SCID issue rate
CK_READ_SETTING(es_qpack_dec_max_size, unsigned, LSQUIC_DF_QPACK_DEC_MAX_SIZE); // QPACK decoder max size
CK_READ_SETTING(es_qpack_dec_max_blocked, unsigned, LSQUIC_DF_QPACK_DEC_MAX_SIZE); // Max blocked for QPACK decoder
CK_READ_SETTING(es_qpack_enc_max_size, unsigned,LSQUIC_DF_QPACK_ENC_MAX_SIZE ); // QPACK encoder max size
CK_READ_SETTING(es_qpack_enc_max_blocked, unsigned, LSQUIC_DF_QPACK_ENC_MAX_BLOCKED); // Max blocked for QPACK encoder
CK_READ_SETTING(es_ecn, int, LSQUIC_DF_ECN); // ECN support (1 = true)
CK_READ_SETTING(es_allow_migration, int, LSQUIC_DF_ALLOW_MIGRATION); // Allow connection migration (1 = true)
CK_READ_SETTING(es_ql_bits, int, LSQUIC_DF_QL_BITS); // QUIC load bits
CK_READ_SETTING(es_spin, int, LSQUIC_DF_SPIN); // Spin bit (1 = true)
CK_READ_SETTING(es_delayed_acks, int, LSQUIC_DF_DELAYED_ACKS); // Delayed acks (1 = true)
CK_READ_SETTING(es_timestamps, int, LSQUIC_DF_TIMESTAMPS); // Timestamps (1 = enabled)
CK_READ_SETTING(es_max_udp_payload_size_rx, int, LSQUIC_DF_MAX_UDP_PAYLOAD_SIZE_RX); // Max UDP payload size (default 1200 bytes)
CK_READ_SETTING(es_grease_quic_bit, int, LSQUIC_DF_GREASE_QUIC_BIT); // Grease QUIC bit (0 = false)
CK_READ_SETTING(es_dplpmtud, int, LSQUIC_DF_DPLPMTUD); // Use DPLPMTU Discovery (1 = true)
CK_READ_SETTING(es_base_plpmtu, unsigned short, LSQUIC_DF_BASE_PLPMTU); // Base Path MTU (default 1200 bytes)
CK_READ_SETTING(es_max_plpmtu, unsigned short, LSQUIC_DF_MAX_PLPMTU); // Max Path MTU (default 1350 bytes)
CK_READ_SETTING(es_mtu_probe_timer, unsigned, LSQUIC_DF_MTU_PROBE_TIMER); // MTU probe timer (10 seconds)
CK_READ_SETTING(es_datagrams, int, LSQUIC_DF_DATAGRAMS); // Datagrams (1 = enabled)

CK_READ_SETTING(es_optimistic_nat, int, LSQUIC_DF_OPTIMISTIC_NAT); // Default: Optimistic NAT disabled (0)
CK_READ_SETTING(es_ext_http_prio, int, LSQUIC_DF_EXT_HTTP_PRIO); // Default: HTTP priority extension enabled (1)
CK_READ_SETTING(es_qpack_experiment, int, LSQUIC_DF_QPACK_EXPERIMENT); // Default: QPACK experiment disabled (0)

CK_READ_SETTING(es_ptpc_periodicity, unsigned, LSQUIC_DF_PTPC_PERIODICITY); // Default: Periodicity of 1000 ms (1 second)
CK_READ_SETTING(es_ptpc_max_packtol, unsigned, LSQUIC_DF_PTPC_MAX_PACKTOL); // Default: Max packet tolerance 100
CK_READ_SETTING(es_ptpc_dyn_target, int, LSQUIC_DF_PTPC_DYN_TARGET); // Default: Dynamic target disabled (0)
CK_READ_SETTING(es_ptpc_target, float, LSQUIC_DF_PTPC_TARGET); // Default: Target value of 1.0
CK_READ_SETTING(es_ptpc_prop_gain, float, LSQUIC_DF_PTPC_PROP_GAIN); // Default: Proportional gain 0.1
CK_READ_SETTING(es_ptpc_int_gain, float, LSQUIC_DF_PTPC_INT_GAIN); // Default: Integral gain 0.05
CK_READ_SETTING(es_ptpc_err_thresh, float, LSQUIC_DF_PTPC_ERR_THRESH); // Default: Error threshold 0.1
CK_READ_SETTING(es_ptpc_err_divisor, float, LSQUIC_DF_PTPC_ERR_DIVISOR); // Default: Error divisor 1.0

CK_READ_SETTING(es_delay_onclose, int, LSQUIC_DF_DELAY_ONCLOSE); // Default: No delay on close (0)
CK_READ_SETTING(es_max_batch_size, unsigned, LSQUIC_DF_MAX_BATCH_SIZE); // Default: Max batch size 100
CK_READ_SETTING(es_check_tp_sanity, int, LSQUIC_DF_CHECK_TP_SANITY); // Default: Check transport parameters sanity enabled (1)
	}
#undef CK_READ_SETTING
#undef CK_READ_SETTING_Str
}