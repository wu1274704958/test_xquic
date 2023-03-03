//
// Created by Administrator on 2023/3/1.
//

#pragma clang diagnostic push
#pragma ide diagnostic ignored "HidingNonVirtualFunction"
#ifndef MQAS_CONNECT_IMPL_HPP
#define MQAS_CONNECT_IMPL_HPP

#define MQAS_CONNECT_IMPL_TEMPLATE_DECL                                                           \
template<typename S>                                                                              \
requires requires{                                                                                \
    requires std::default_initializable<S>;                                                       \
    requires std::is_base_of_v<IStream, S>;                                                       \
}

namespace mqas::core{

    MQAS_CONNECT_IMPL_TEMPLATE_DECL
    void Connect<S>::init(::lsquic_conn_t *conn, engine_cxt *engine_cxt) {
        IConnect::init(conn, engine_cxt);
        connect_cxt_ = {};
        connect_cxt_.engine_cxt_ = engine_cxt_;
        connect_cxt_.has_stream = std::bind_front(&Connect<S>::has_stream,this);
        connect_cxt_.write_stream = std::bind_front(&Connect<S>::write_stream,this);
    }

    MQAS_CONNECT_IMPL_TEMPLATE_DECL
    void Connect<S>::on_close()
    {

    }
    MQAS_CONNECT_IMPL_TEMPLATE_DECL
    void Connect<S>::on_new_stream(::lsquic_stream_t* lsquic_stream)
    {
        const auto key = reinterpret_cast<size_t>(lsquic_stream);
        stream_map_.emplace(key,std::make_shared<S>());
        stream_map_[key]->on_init(lsquic_stream,&connect_cxt_);
        if(on_new_stream_cb_)
        {
            on_new_stream_cb_(stream_map_[key]);
            on_new_stream_cb_ = nullptr;
        }
    }
    MQAS_CONNECT_IMPL_TEMPLATE_DECL
    void Connect<S>::on_stream_read(::lsquic_stream_t* lsquic_stream)
    {
        const auto key = reinterpret_cast<size_t>(lsquic_stream);
        if(stream_map_.contains(key))
            stream_map_[key]->do_read();
    }
    MQAS_CONNECT_IMPL_TEMPLATE_DECL
    void Connect<S>::on_stream_write(::lsquic_stream_t* lsquic_stream)
    {
        const auto key = reinterpret_cast<size_t>(lsquic_stream);
        if(stream_map_.contains(key))
            stream_map_[key]->do_write();
    }
    MQAS_CONNECT_IMPL_TEMPLATE_DECL
    void Connect<S>::on_stream_close(::lsquic_stream_t* lsquic_stream)
    {
        const auto key = reinterpret_cast<size_t>(lsquic_stream);
        if(stream_map_.contains(key))
            stream_map_[key]->on_close();
        stream_map_.erase(key);
    }
    MQAS_CONNECT_IMPL_TEMPLATE_DECL
    void Connect<S>::on_stream_reset(lsquic_stream_t* s, int how)
    {
        const auto key = reinterpret_cast<size_t>(s);
        if(stream_map_.contains(key))
            stream_map_[key]->on_reset(static_cast<StreamAspect>((unsigned char)how));
    }
    MQAS_CONNECT_IMPL_TEMPLATE_DECL
    bool Connect<S>::has_stream(lsquic_stream_t*s) const
    {
        return stream_map_.contains(reinterpret_cast<size_t>(s));
    }
    MQAS_CONNECT_IMPL_TEMPLATE_DECL
    bool Connect<S>::write_stream(::lsquic_stream_t* s,const std::span<uint8_t>& data)
    {
        const auto key = reinterpret_cast<size_t>(s);
        if(stream_map_.contains(key))
            return stream_map_[key]->write(data);
        return false;
    }
    MQAS_CONNECT_IMPL_TEMPLATE_DECL
    void Connect<S>::make_stream(std::function<void(std::weak_ptr<S>)> cb)
    {
        this->on_new_stream_cb_ = std::move(cb);
        IConnect::make_stream();
    }
    MQAS_CONNECT_IMPL_TEMPLATE_DECL
    void Connect<S>::close()
    {
        for(auto& s : stream_map_)
        {
            s.second->close();
        }
        IConnect::close();
    }
}
#undef  MQAS_CONNECT_IMPL_TEMPLATE_DECL
#endif //MQAS_CONNECT_IMPL_HPP

#pragma clang diagnostic pop