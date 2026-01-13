//
// Created by Administrator on 2023/3/1.
//

#pragma clang diagnostic push
#pragma ide diagnostic ignored "HidingNonVirtualFunction"
#ifndef MQAS_CONNECT_IMPL_HPP
#define MQAS_CONNECT_IMPL_HPP

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
    }

    MQAS_CONNECT_IMPL_TEMPLATE_DECL
    void Connect<S>::on_close()
    {
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
        _temp_new_stream_result = lsquic_stream;
        on_new_stream_signal.emit(_stream_map[key]);
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
            _stream_map[key]->on_close();
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
    std::shared_ptr<S> Connect<S>::make_stream()
    {
        _temp_new_stream_result = nullptr;
        IConnect::make_stream();
        if (_temp_new_stream_result == nullptr)
            return nullptr;
        else
            return _stream_map.at(reinterpret_cast<size_t>(_temp_new_stream_result));
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
#endif //MQAS_CONNECT_IMPL_HPP

#pragma clang diagnostic pop
