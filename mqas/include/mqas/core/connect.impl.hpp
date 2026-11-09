//
// Created by Administrator on 2023/3/1.
//

#ifndef MQAS_CONNECT_IMPL_HPP
#define MQAS_CONNECT_IMPL_HPP

#ifdef __clang__
#pragma clang diagnostic push
#pragma ide diagnostic ignored "HidingNonVirtualFunction"
#endif

#define MQAS_CONNECT_IMPL_TEMPLATE_DECL                                                             \
template<typename S>                                                                                \
requires requires{                                                                                  \
    requires std::is_default_constructible_v<S>;                                                    \
    requires std::is_base_of_v<IStream, S>;                                                         \
}

namespace mqas::core
{
    MQAS_CONNECT_IMPL_TEMPLATE_DECL
    void Connect<S>::init(::lsquic_conn_t* conn, std::shared_ptr<engine_cxt> cxt)
    {
        IConnect::init(conn, cxt);
        this->_connect_cxt = {0};
        this->_connect_cxt.engine_cxt_ = engine_cxt_;
        this->_connect_cxt.has_stream = std::bind_front(&Connect<S>::has_stream, this);
        this->_connect_cxt.write_stream = std::bind_front(&Connect<S>::write_stream, this);
        this->pending_stream_cancel_limit_ = std::bind_front(&Connect<S>::pending_stream_cancel_limit, this);
        this->on_pending_streams_cancelled_ = std::bind_front(&Connect<S>::on_pending_streams_cancelled, this);
    }

    MQAS_CONNECT_IMPL_TEMPLATE_DECL
    unsigned int Connect<S>::pending_stream_cancel_limit(unsigned int count) const
    {
        return static_cast<unsigned int>(std::min<size_t>(count, _stream_requests.size()));
    }

    MQAS_CONNECT_IMPL_TEMPLATE_DECL
    void Connect<S>::on_pending_streams_cancelled(unsigned int count)
    {
        decltype(_stream_requests) cancelled;
        while (count-- > 0 && !_stream_requests.empty()) {
            cancelled.push_front(std::move(_stream_requests.back()));
            _stream_requests.pop_back();
        }
        // Remove the whole batch before callbacks, which may enqueue requests.
        for (auto& request : cancelled) {
            if (request.callback) request.callback(request.id, {});
        }
    }

    MQAS_CONNECT_IMPL_TEMPLATE_DECL
    void Connect<S>::on_close()
    {
        _stream_requests_closed = true;
        auto pending = std::move(_stream_requests);
        _stream_requests.clear();
        for (auto& request : pending) {
            if (request.callback) request.callback(request.id, {});
        }
        IConnect::on_close();
    }

    MQAS_CONNECT_IMPL_TEMPLATE_DECL
    void Connect<S>::on_new_stream(::lsquic_stream_t* lsquic_stream)
    {
        if (lsquic_stream == nullptr)
            return;
        const auto key = reinterpret_cast<size_t>(lsquic_stream);
        _stream_map.emplace(key, std::make_shared<S>());
        _stream_map[key]->on_init(lsquic_stream, &_connect_cxt, this->weak_from_this());
        auto stream = _stream_map[key];
        const auto id = ::lsquic_stream_id(lsquic_stream);
        const bool server = (static_cast<unsigned>(engine_cxt_->engine_flags) &
            static_cast<unsigned>(EngineFlags::Server)) != 0;
        const bool local_bidi = (id & 2u) == 0 && ((id & 1u) != 0) == server;
        if (local_bidi) {
            struct CallbackGuard {
                unsigned int& depth;
                explicit CallbackGuard(unsigned int& d) : depth(d) { ++depth; }
                ~CallbackGuard() { --depth; }
            } guard(this->stream_creation_callback_depth_);
            if (!_stream_requests.empty()) {
                auto request = std::move(_stream_requests.front());
                _stream_requests.pop_front();
                if (request.callback) request.callback(request.id, stream);
            }
            on_new_stream_signal.emit(stream);
        } else {
            on_new_stream_signal.emit(stream);
        }
    }

    MQAS_CONNECT_IMPL_TEMPLATE_DECL
    void Connect<S>::on_stream_read(::lsquic_stream_t* lsquic_stream)
    {
        const auto key = reinterpret_cast<size_t>(lsquic_stream);
        if (_stream_map.contains(key))
            _stream_map[key]->do_read();
    }

    MQAS_CONNECT_IMPL_TEMPLATE_DECL
    void Connect<S>::on_stream_write(::lsquic_stream_t* lsquic_stream)
    {
        const auto key = reinterpret_cast<size_t>(lsquic_stream);
        if (_stream_map.contains(key))
            _stream_map[key]->do_write();
    }

    MQAS_CONNECT_IMPL_TEMPLATE_DECL
    void Connect<S>::on_stream_close(::lsquic_stream_t* lsquic_stream)
    {
        const auto key = reinterpret_cast<size_t>(lsquic_stream);
        if (_stream_map.contains(key))
        {
            auto& stream = _stream_map[key];
            stream->on_close();
            on_stream_close_signal.emit(stream);
        }
        _stream_map.erase(key);

    }

    MQAS_CONNECT_IMPL_TEMPLATE_DECL
    void Connect<S>::on_stream_reset(lsquic_stream_t* s, int how)
    {
        const auto key = reinterpret_cast<size_t>(s);
        if (_stream_map.contains(key))
            _stream_map[key]->on_reset(static_cast<StreamAspect>(static_cast<unsigned char>(how)));
    }

    MQAS_CONNECT_IMPL_TEMPLATE_DECL
    bool Connect<S>::has_stream(lsquic_stream_t* s) const
    {
        return _stream_map.contains(reinterpret_cast<size_t>(s));
    }

    MQAS_CONNECT_IMPL_TEMPLATE_DECL
    bool Connect<S>::write_stream(::lsquic_stream_t* s, const std::span<uint8_t>& data)
    {
        const auto key = reinterpret_cast<size_t>(s);
        if (this->_stream_map.contains(key))
            return this->_stream_map[key]->write(data);
        return false;
    }

    MQAS_CONNECT_IMPL_TEMPLATE_DECL
    typename Connect<S>::StreamRequestId Connect<S>::make_stream(
        std::function<void(StreamRequestId, std::shared_ptr<S>)> on_ready)
    {
        if (!on_ready || _stream_requests_closed || is_closed() || _next_stream_request == 0) return 0;
        const auto rid = _next_stream_request++;
        _stream_requests.push_back({rid, std::move(on_ready)});
        IConnect::make_stream();
        return rid;
    }

    MQAS_CONNECT_IMPL_TEMPLATE_DECL
    void Connect<S>::close()
    {
        for (auto& s : this->_stream_map)
        {
            s.second->close();
        }
        IConnect::close();
    }

    MQAS_CONNECT_IMPL_TEMPLATE_DECL
    std::shared_ptr<S> Connect<S>::get_stream(::lsquic_stream_t* s) const
    {
        const auto key = reinterpret_cast<size_t>(s);
        if (this->_stream_map.contains(key))
            return this->_stream_map.at(key);
        return {};
    }
}
#undef  MQAS_CONNECT_IMPL_TEMPLATE_DECL
#ifdef __clang__
#pragma clang diagnostic pop
#endif
#endif //MQAS_CONNECT_IMPL_HPP


