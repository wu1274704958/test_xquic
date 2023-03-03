#pragma once

#define ENGINE_TEMPLATE_DECL						 \
template<typename C>								 \
requires requires{									 \
	requires std::default_initializable<C>;			 \
	requires std::is_base_of_v<mqas::core::IConnect, C>;		 \
}
ENGINE_TEMPLATE_DECL
void mqas::core::engine<C>::init(void* engine_base_ptr) //override
{
	IEngine::init(engine_base_ptr);
    engine_cxt_.engine_flags = get_engine_flags();
	engine_cxt_.process_conns = std::bind(&engine<C>::process_conns,this);
    engine_cxt_.write_datagram = std::bind_front(&engine<C>::write_datagram,this);
    engine_cxt_.write_stream = std::bind_front(&engine<C>::write_stream,this);
    engine_cxt_.has_stream = std::bind_front(&engine<C>::has_stream,this);
}

ENGINE_TEMPLATE_DECL
std::weak_ptr<C> mqas::core::engine<C>::connect(const ::sockaddr & addr, ::lsquic_version ver, const char* hostname, unsigned short base_plpmtu,
	const unsigned char* sess_resume, size_t sess_resume_len, const unsigned char* token, size_t token_sz)
{
	const auto p = static_cast<engine_base<engine<C>>*>(engine_base_ptr_);
	auto conn = p->connect(addr,ver,hostname,base_plpmtu,sess_resume,sess_resume_len,token,token_sz);
	if(conn == nullptr) return {};
	return add(conn);
}
ENGINE_TEMPLATE_DECL
bool mqas::core::engine<C>::contain(::lsquic_conn_t* conn) const
{
	return conn_map_.contains(reinterpret_cast<size_t>(conn));
}
ENGINE_TEMPLATE_DECL
std::weak_ptr<C> mqas::core::engine<C>::add(::lsquic_conn_t* conn)
{
	if(!contain(conn))
	{
		conn_map_.emplace(reinterpret_cast<size_t>(conn),std::make_shared<C>());
		auto c = conn_map_[reinterpret_cast<size_t>(conn)];
		c->init(conn,&engine_cxt_);
		return c;
	}
	return conn_map_[reinterpret_cast<size_t>(conn)];
}
ENGINE_TEMPLATE_DECL
void mqas::core::engine<C>::on_new_conn([[maybe_unused]] void* stream_if_ctx, lsquic_conn_t* lsquic_conn)
{
	add(lsquic_conn);
}
ENGINE_TEMPLATE_DECL
void mqas::core::engine<C>::on_conn_closed(lsquic_conn_t* lsquic_conn)
{
	const auto key = reinterpret_cast<size_t>(lsquic_conn);
	if (conn_map_.contains(key))
	{
		conn_map_[key]->on_close();
		conn_map_.erase(key);
	}
}
ENGINE_TEMPLATE_DECL
void mqas::core::engine<C>::on_new_stream([[maybe_unused]] void* stream_if_ctx, lsquic_stream_t* lsquic_stream)
{
	const auto key = reinterpret_cast<size_t>(::lsquic_stream_conn(lsquic_stream));
	if (conn_map_.contains(key))
	{
		conn_map_[key]->on_new_stream(lsquic_stream);
	}
}
ENGINE_TEMPLATE_DECL
void mqas::core::engine<C>::on_read(lsquic_stream_t* lsquic_stream, [[maybe_unused]] lsquic_stream_ctx_t* lsquic_stream_ctx)
{
	const auto key = reinterpret_cast<size_t>(::lsquic_stream_conn(lsquic_stream));
	if (conn_map_.contains(key))
	{
		conn_map_[key]->on_stream_read(lsquic_stream);
	}
}
ENGINE_TEMPLATE_DECL
void mqas::core::engine<C>::on_write(lsquic_stream_t* lsquic_stream, [[maybe_unused]] lsquic_stream_ctx_t* lsquic_stream_ctx)
{
	const auto key = reinterpret_cast<size_t>(::lsquic_stream_conn(lsquic_stream));
	if (conn_map_.contains(key))
	{
		conn_map_[key]->on_stream_write(lsquic_stream);
	}
}
ENGINE_TEMPLATE_DECL
void mqas::core::engine<C>::on_close(lsquic_stream_t* lsquic_stream, [[maybe_unused]] lsquic_stream_ctx_t* lsquic_stream_ctx)
{
	const auto key = reinterpret_cast<size_t>(::lsquic_stream_conn(lsquic_stream));
	if (conn_map_.contains(key))
	{
		conn_map_[key]->on_stream_close(lsquic_stream);
	}
}

//optional callback
ENGINE_TEMPLATE_DECL
void mqas::core::engine<C>::on_goaway_received(lsquic_conn_t* c)
{
	const auto key = reinterpret_cast<size_t>(c);
	if (conn_map_.contains(key))
	{
		conn_map_[key]->on_goaway_received();
	}
}
ENGINE_TEMPLATE_DECL
ssize_t mqas::core::engine<C>::on_dg_write(lsquic_conn_t* c, void* buf, size_t sz)
{
	const auto key = reinterpret_cast<size_t>(c);
	if (conn_map_.contains(key))
	{
		return conn_map_[key]->on_dg_write(buf,sz);
	}
	return 0;
}
ENGINE_TEMPLATE_DECL
void mqas::core::engine<C>::on_datagram(lsquic_conn_t* c, const void* buf, size_t sz)
{
	const auto key = reinterpret_cast<size_t>(c);
	if (conn_map_.contains(key))
	{
		conn_map_[key]->on_datagram(buf, sz);
	}
}
ENGINE_TEMPLATE_DECL
void mqas::core::engine<C>::on_hsk_done(lsquic_conn_t* c, enum lsquic_hsk_status s)
{
	const auto key = reinterpret_cast<size_t>(c);
	if (conn_map_.contains(key))
	{
		conn_map_[key]->on_hsk_done(s);
	}
}
ENGINE_TEMPLATE_DECL
void mqas::core::engine<C>::on_new_token(lsquic_conn_t* c, const unsigned char* token, size_t token_size)
{
	const auto key = reinterpret_cast<size_t>(c);
	if (conn_map_.contains(key))
	{
		conn_map_[key]->on_new_token(token,token_size);
	}
}
ENGINE_TEMPLATE_DECL
void mqas::core::engine<C>::on_reset(lsquic_stream_t* s, [[maybe_unused]] lsquic_stream_ctx_t* h, int how)
{
	const auto key = reinterpret_cast<size_t>(::lsquic_stream_conn(s));
	if (conn_map_.contains(key))
	{
		conn_map_[key]->on_stream_reset(s, how);
	}
}
ENGINE_TEMPLATE_DECL
void mqas::core::engine<C>::on_conncloseframe_received(lsquic_conn_t* c, int app_error, uint64_t error_code, const char* reason, int reason_len)
{
	const auto key = reinterpret_cast<size_t>(c);
	if (conn_map_.contains(key))
	{
		conn_map_[key]->on_conncloseframe_received(app_error,error_code,reason,reason_len);
	}
}
ENGINE_TEMPLATE_DECL
void mqas::core::engine<C>::process_conns() const
{
	const auto p = static_cast<engine_base<engine<C>>*>(engine_base_ptr_);
	p->process_conns();
}

ENGINE_TEMPLATE_DECL
bool mqas::core::engine<C>::write_datagram(::lsquic_conn_t* conn, const std::span<uint8_t> &d) {
    const auto key = reinterpret_cast<size_t >(conn);
    if(conn_map_.contains(key))
    {
        conn_map_[key]->write_datagram(d);
        return true;
    }
    return false;
}

ENGINE_TEMPLATE_DECL
bool mqas::core::engine<C>::write_stream(::lsquic_conn_t* conn,lsquic_stream_t* stream,const std::span<uint8_t>&d)
{
    const auto key = reinterpret_cast<size_t >(conn);
    if(conn_map_.contains(key))
    {
        conn_map_[key]->write_stream(stream,d);
        return true;
    }
    return false;
}

ENGINE_TEMPLATE_DECL
bool mqas::core::engine<C>::has_stream(lsquic_conn_t *conn, lsquic_stream_t *stream) const {
    const auto key = reinterpret_cast<size_t >(conn);
    if(conn_map_.contains(key))
    {
        return conn_map_.at(key) -> has_stream(stream);
    }
    return false;
}

ENGINE_TEMPLATE_DECL
mqas::core::EngineFlags mqas::core::engine<C>::get_engine_flags() const {
    const auto p = static_cast<engine_base<engine<C>>*>(engine_base_ptr_);
    return p->engine_flags_;
}

ENGINE_TEMPLATE_DECL
void mqas::core::engine<C>::close()
{
    for(auto& c : conn_map_)
    {
        c.second->close();
    }
    process_conns();
}

#undef ENGINE_TEMPLATE_DECL