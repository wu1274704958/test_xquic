#pragma once

namespace mqas::core {

	#define SUB_ENGINE_TEMPLATE_DECL					\
	template<typename E,typename ED>					\
	requires requires									\
	{													\
		requires IsVaildEngineDriver<ED>;				\
		requires std::is_default_constructible_v<E>;	\
		requires std::is_base_of_v<IEngine, E>;			\
	}

	SUB_ENGINE_TEMPLATE_DECL
	sub_engine<E,ED>::sub_engine(io::Context& c):io_cxt(c), _socket(nullptr),
		_proc_conns_timer(nullptr), _engine_flags(EngineFlags::None), _local_addr({}),
		_lsquic_engine_api({})
	{}

	SUB_ENGINE_TEMPLATE_DECL
	sub_engine<E,ED>::sub_engine(sub_engine&& oth) noexcept :io_cxt(oth.io_cxt), _socket(std::move(oth._socket)),
		_proc_conns_timer(oth._proc_conns_timer), _engine_flags(oth._engine_flags), _local_addr(oth._local_addr),
		_lsquic_engine_api(oth._lsquic_engine_api) 
	{
		oth._proc_conns_timer = nullptr;
	}

	SUB_ENGINE_TEMPLATE_DECL
	void sub_engine<E, ED>::init(const char* conf_file, core::EngineFlags engine_flags, std::shared_ptr<io::UdpSocket> socket) noexcept(false)
	{
		if (!ED::instance()->initialization(conf_file))
			throw std::runtime_error("Initialize engine driver failed!");
		
		_engine_extern = std::make_shared<E>();

		init_config(conf_file);
		_engine_flags = engine_flags;

		init_socket(std::move(socket));
		init_engine_core();
		init_context();
		
		if (!ED::instance()->register_engine(this, _engine))
			throw std::runtime_error("Register in engine driver failed!");
		_engine_extern->init(context);
		init_timer();
	}

	SUB_ENGINE_TEMPLATE_DECL
	void sub_engine<E, ED>::init_config(const char* conf_file)
	{
		_conf_origin = std::make_shared<toml::value>(toml::parse(conf_file));
		_conf = std::make_shared<engine_config>(toml::find<engine_config>(*_conf_origin, "engine_config"));
	}

	SUB_ENGINE_TEMPLATE_DECL
	bool sub_engine<E, ED>::has_engine_setting() const
	{
		return _conf_origin != nullptr && _conf_origin->contains("lsquic_settings");
	}

	SUB_ENGINE_TEMPLATE_DECL
	void sub_engine<E, ED>::init_engine_core()
	{
		_lsquic_engine_api.ea_packets_out_ctx = _socket.get();
		ED::instance()->fill_lsquic_engine_api(this, _engine_flags, _lsquic_engine_api,*_conf);

		if (has_engine_setting())
		{
			engine_driver::settings_from_toml(_conf->lsquic_settings, _conf_origin->at("lsquic_settings"));
			_lsquic_engine_api.ea_settings = &_conf->lsquic_settings;
		}

		if (contain<uint32_t>(_engine_flags, EngineFlags::Server))
		{ 
			_lsquic_engine_api.ea_get_ssl_ctx = on_get_ssl_ctx;
			_ssl_ctx = ED::instance()->get_ssl_or_generate(_conf->ssl_cert_path,_conf->ssl_key_path,_conf->alpn);
		}
		if (_engine_flags == EngineFlags::None)
		{
			_lsquic_engine_api.ea_alpn = _conf->alpn.c_str();
			if (_conf->alpn.empty())
				LOG(WARNING) << "Client alpn is empty!";
		}

		if (has_engine_setting())
		{
			char errbuf[512] = { 0 };
			if (0 != ::lsquic_engine_check_settings(&_conf->lsquic_settings,
				static_cast<unsigned>(_engine_flags), errbuf, sizeof(errbuf)))
			{
				LOG(ERROR) << "invalid settings: " << errbuf;
				throw std::runtime_error("Invalid settings look log file!");
			}
		}

		_engine_extern->on_new_lsquic_engine(_lsquic_engine_api, _engine_flags);
		_engine = lsquic_engine_new(static_cast<unsigned>(_engine_flags), &_lsquic_engine_api);
		if (_engine == nullptr)
		{
			LOG(ERROR) << "Create engine failed!";
			throw std::runtime_error("Create engine failed!");
		}
		LOG(INFO) << "Create engine success!";

		_engine_extern->on_init_config(_conf_origin);
		_engine_extern->on_init_socket(_socket);
	}
	SUB_ENGINE_TEMPLATE_DECL
	void sub_engine<E, ED>::init_socket(std::shared_ptr<io::UdpSocket> sock)
	{
		if(sock == nullptr)
		{ 
			_socket = io_cxt.make_shared<io::UdpSocket>();
			sockaddr addr{};
			io::Ip::str2addr_ipv4(_conf->bind_ip.c_str(), _conf->port, addr);
			_socket->bind(addr, UV_UDP_REUSEADDR);
		}else
			_socket = std::move(sock);
		_socket->get_sock_addr(this->_local_addr);

	}
	SUB_ENGINE_TEMPLATE_DECL
	void sub_engine<E, ED>::init_timer()
	{
		_proc_conns_timer = io_cxt.make_handle<io::Timer>();
	}

	SUB_ENGINE_TEMPLATE_DECL
	void sub_engine<E, ED>::init_context()
	{
		context = std::make_shared<engine_cxt>(io_cxt, _local_addr);
		context->engine_core = _engine;
		context->engine_flags = _engine_flags;
		context->process_conns = std::bind(&sub_engine::process_conns, this);
		context->process_conns_lazy = std::bind(&sub_engine::process_conns_lazy, this);
	}

	SUB_ENGINE_TEMPLATE_DECL
	void sub_engine<E, ED>::close_timer()
	{
		if (_proc_conns_timer)
		{
			_proc_conns_timer->stop();
			io_cxt.del_handle(_proc_conns_timer);
			_proc_conns_timer = nullptr;
		}
	}
	SUB_ENGINE_TEMPLATE_DECL
	void sub_engine<E, ED>::close()
	{
		if (_engine_extern)
			_engine_extern->close();
	}
	SUB_ENGINE_TEMPLATE_DECL
	void sub_engine<E, ED>::wait_all_connect_closed()
	{
		close();
		while (_engine_extern && _engine_extern->connect_count() > 0)
			io_cxt.run(mqas::io::Context::RunMode::ONCE);
	}
	SUB_ENGINE_TEMPLATE_DECL
	sub_engine<E, ED>::~sub_engine()
	{
		close_timer();
		if(_engine != nullptr)
		{ 
			::lsquic_engine_destroy(_engine);
			ED::instance()->unregister_engine(this,_engine);
			_ssl_ctx = ED::instance()->destroy_ssl_ctx(_ssl_ctx);
			if (_recv_connection.connected())
				_recv_connection.disconnect();
			_engine = nullptr;
		}
	}
	SUB_ENGINE_TEMPLATE_DECL
	void sub_engine<E, ED>::process_conns() const
	{
		_proc_conns_timer->stop();
		if (_engine == nullptr) return;
		int diff;
		::lsquic_engine_process_conns(_engine);
		if (::lsquic_engine_earliest_adv_tick(_engine, &diff)) {
			int timeout;
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
			_proc_conns_timer->start([this](io::Timer* t)
				{
					this->process_conns();
				}, timeout, 0);
		}
	}
	SUB_ENGINE_TEMPLATE_DECL
	void sub_engine<E, ED>::process_conns_lazy() const
	{
		_proc_conns_timer->stop();
		_proc_conns_timer->start([this](io::Timer* t)
			{
				this->process_conns();
			}, 0, 0);
	}
	SUB_ENGINE_TEMPLATE_DECL
	void sub_engine<E, ED>::start_recv()
	{
		if (_recv_connection.connected())
			return;
		_socket->recv_start();
		auto engine_driver = ED::instance().get();
		_recv_connection = _socket->on_recv_signal.connect([this, engine_driver](io::UdpSocket* sock, const std::optional<std::span<uint8_t>>& buf, ssize_t nread,
			const sockaddr* addr, unsigned flags)
			{
				if (nread == 0 || addr == nullptr)
					return;
				if (nread < 0) {
					// there seems to be no way to get an error code here (none of the udp tests do)
					LOG(ERROR) << "udp recv error unexpected nread = " << nread;
					return;
				}
				if (_engine_extern->on_recv(buf, nread, addr, flags))
				{
					const int ret = ::lsquic_engine_packet_in(_engine, reinterpret_cast<const unsigned char*>(buf->data()), nread,
						&this->_local_addr, addr, (void*)&(_peer_context_mgr.get_or_create(addr,this)), 0);
					//LOG(INFO) << "@V@ lsquic_engine_packet_in ret = " << ret;
					this->process_conns();
				}
			});
	}

	SUB_ENGINE_TEMPLATE_DECL
	std::shared_ptr<E> sub_engine<E, ED>::get_engine() const
	{
		return _engine_extern;
	}

	SUB_ENGINE_TEMPLATE_DECL
	ssl_ctx_st* sub_engine<E, ED>::on_get_ssl_ctx(void* peer_ctx, const sockaddr* local)
	{
		auto ptr = static_cast<peer_context<sub_engine<E, ED>>*>(peer_ctx);
		return ptr->engine->_ssl_ctx;
	}
	///// lsquic event
	SUB_ENGINE_TEMPLATE_DECL
	void sub_engine<E, ED>::on_new_conn_s(void* stream_if_ctx, lsquic_conn_t* lsquic_conn) 
	{
		MQAS_DBG("new conn " << lsquic_conn);
		_engine_extern->on_new_conn(stream_if_ctx, lsquic_conn);
	}
	SUB_ENGINE_TEMPLATE_DECL
	void sub_engine<E, ED>::on_conn_closed_s(lsquic_conn_t* lsquic_conn) 
	{
		MQAS_DBG("conn on close " << lsquic_conn);
		_engine_extern->on_conn_closed(lsquic_conn);
	}
	SUB_ENGINE_TEMPLATE_DECL
	void sub_engine<E, ED>::on_new_stream_s(void* stream_if_ctx, lsquic_stream_t* lsquic_stream) 
	{
		MQAS_DBG("new stream " << lsquic_stream);
		_engine_extern->on_new_stream(stream_if_ctx, lsquic_stream);
	}
	SUB_ENGINE_TEMPLATE_DECL
	void sub_engine<E, ED>::on_read_s(lsquic_stream_t* lsquic_stream, lsquic_stream_ctx_t* lsquic_stream_ctx)
	{
		MQAS_DBG("stream on read " << lsquic_stream);
		_engine_extern->on_read(lsquic_stream, lsquic_stream_ctx);
	}
	SUB_ENGINE_TEMPLATE_DECL
	void sub_engine<E, ED>::on_write_s(lsquic_stream_t* lsquic_stream, lsquic_stream_ctx_t* lsquic_stream_ctx)
	{
		MQAS_DBG("stream on write " << lsquic_stream);
		_engine_extern->on_write(lsquic_stream, lsquic_stream_ctx);
	}
	SUB_ENGINE_TEMPLATE_DECL
	void sub_engine<E, ED>::on_close_s(lsquic_stream_t* lsquic_stream, lsquic_stream_ctx_t* lsquic_stream_ctx)
	{
		MQAS_DBG("stream on close " << lsquic_stream);
		_engine_extern->on_close(lsquic_stream, lsquic_stream_ctx);
	}
	//optional
	SUB_ENGINE_TEMPLATE_DECL
	void sub_engine<E, ED>::on_goaway_received(lsquic_conn_t* c)
	{
		MQAS_DBG("conn on goaway received " << c);
		_engine_extern->on_goaway_received(c);
	}
	SUB_ENGINE_TEMPLATE_DECL
	ssize_t sub_engine<E, ED>::on_dg_write(lsquic_conn_t* c, void* buf, size_t buf_sz) {
		MQAS_DBG("conn on dg write " << c << " buf size = " << buf_sz);
		return _engine_extern->on_dg_write(c, buf, buf_sz);
	}
	SUB_ENGINE_TEMPLATE_DECL
	void sub_engine<E, ED>::on_datagram(lsquic_conn_t* c, const void* buf, size_t buf_sz)
	{
		MQAS_DBG("conn on datagram " << c << " buf size = " << buf_sz);
		_engine_extern->on_datagram(c, buf, buf_sz);
	}
	SUB_ENGINE_TEMPLATE_DECL
	void sub_engine<E, ED>::on_hsk_done(lsquic_conn_t* c, enum lsquic_hsk_status s)
	{
		MQAS_DBG("conn on hsk done " << c << " status = " << s);
		_engine_extern->on_hsk_done(c, s);
	}
	SUB_ENGINE_TEMPLATE_DECL
	void sub_engine<E, ED>::on_new_token(lsquic_conn_t* c, const unsigned char* token, size_t token_size)
	{
		MQAS_DBG("conn on new token " << c << " token = " << token);
		_engine_extern->on_new_token(c, token, token_size);
	}
	SUB_ENGINE_TEMPLATE_DECL
	void sub_engine<E, ED>::on_reset(lsquic_stream_t* s, lsquic_stream_ctx_t* h, int how)
	{
		MQAS_DBG("stream on reset " << s << " how = " << how);
		_engine_extern->on_reset(s, h, how);
	}
	SUB_ENGINE_TEMPLATE_DECL
	void sub_engine<E, ED>::on_conncloseframe_received(lsquic_conn_t* c, int app_error, uint64_t error_code, const char* reason, int reason_len)
	{
		MQAS_DBG("conn on conncloseframe received " << c << " error_code = " << error_code << " reason " << reason);
		_engine_extern->on_conncloseframe_received(c, app_error, error_code, reason, reason_len);
	}
	/////

	#undef SUB_ENGINE_TEMPLATE_DECL
	#undef MQAS_DBG
}