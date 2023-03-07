//
// Created by Administrator on 2023/3/1.
//

#pragma clang diagnostic push
#pragma ide diagnostic ignored "HidingNonVirtualFunction"
#pragma clang diagnostic ignored "-Wunused-value"
#ifndef MQAS_STREAM_IMPL_HPP
#define MQAS_STREAM_IMPL_HPP


#define MQAS_STREAM_IMPL_TEMPLATE_DECL                                  \
template<typename ... S>                                                \
requires requires{                                                      \
    requires (variability_stream_require<S> && ...);                    \
}

namespace mqas::core{
    MQAS_STREAM_IMPL_TEMPLATE_DECL
    size_t StreamVariant<S...>::do_read() {
        if(stream_tag_ == 0)
        {
            auto ret = IStream::do_read();
            if(ret > 0) {
                const auto span = IStream::read_all_not_move();
                if (const auto read_len = on_read(span); read_len > 0)
                    IStream::move_read_pos_uncheck(read_len);
            }
            return ret;
        }else{
            size_t ret = 0;
            ((std::holds_alternative<S>(stream_var_) && (ret = do_read_curr(std::get<S>(stream_var_)))),...);
            return ret;
        }
    }
    MQAS_STREAM_IMPL_TEMPLATE_DECL
    void StreamVariant<S...>::do_write()
    {
        if(stream_tag_ == 0)
        {
            IStream::do_write();
        }else{
            ((std::holds_alternative<S>(stream_var_) && (std::get<S>(stream_var_).do_write(),false)),...);
        }
    }
    MQAS_STREAM_IMPL_TEMPLATE_DECL
    void StreamVariant<S...>::on_close()
    {
        if(stream_tag_ == 0)
        {
            IStream::on_close();
        }else{
            ((std::holds_alternative<S>(stream_var_) && (std::get<S>(stream_var_).on_close(),false)),...);
        }
    }
    MQAS_STREAM_IMPL_TEMPLATE_DECL
    void StreamVariant<S...>::on_reset(StreamAspect how)
    {
        if(stream_tag_ == 0)
        {
            IStream::on_reset(how);
        }else{
            ((std::holds_alternative<S>(stream_var_) && (std::get<S>(stream_var_).on_reset(how),false)),...);
        }
    }
    MQAS_STREAM_IMPL_TEMPLATE_DECL
    bool StreamVariant<S...>::write(const std::span<uint8_t>& d)
    {
        if(stream_tag_ == 0)
        {
            return IStream::write(d);
        }else{
            bool ret = false;
            ((std::holds_alternative<S>(stream_var_) && (ret = std::get<S>(stream_var_).write(d))),...);
            return ret;
        }
    }
    MQAS_STREAM_IMPL_TEMPLATE_DECL
    bool StreamVariant<S...>::want_read(bool f) const
    {
        if(stream_tag_ == 0)
        {
            return IStream::want_read();
        }else{
            bool ret = false;
            ((std::holds_alternative<S>(stream_var_) && ret = std::get<S>(stream_var_).want_read(f)),...);
            return ret;
        }
    }
    MQAS_STREAM_IMPL_TEMPLATE_DECL
    bool StreamVariant<S...>::want_write(bool f) const
    {
        if(stream_tag_ == 0)
        {
            return IStream::want_write();
        }else{
            bool ret = false;
            ((std::holds_alternative<S>(stream_var_) && ret = std::get<S>(stream_var_).want_write(f)),...);
            return ret;
        }
    }

    MQAS_STREAM_IMPL_TEMPLATE_DECL
    void* StreamVariant<S...>::get_cxt() const
    {
        if(stream_tag_ == 0)
        {
            return IStream::get_cxt();
        }else{
            void* ret = nullptr;
            ((std::holds_alternative<S>(stream_var_) && ret = std::get<S>(stream_var_).get_cxt()),...);
            return ret;
        }
    }
    MQAS_STREAM_IMPL_TEMPLATE_DECL
    void StreamVariant<S...>::set_cxt(void* c)
    {
        if(stream_tag_ == 0)
        {
            IStream::set_cxt(c);
        }else{
            ((std::holds_alternative<S>(stream_var_) && (std::get<S>(stream_var_).set_cxt(c),false)),...);
        }
    }
    MQAS_STREAM_IMPL_TEMPLATE_DECL
    void StreamVariant<S...>::clear_read_buf()
    {
        if(stream_tag_ == 0)
        {
            IStream::clear_read_buf();
        }else{
            ((std::holds_alternative<S>(stream_var_) && (std::get<S>(stream_var_).clear_read_buf(),false)),...);
        }
    }
    MQAS_STREAM_IMPL_TEMPLATE_DECL
    bool StreamVariant<S...>::has_unread_data() const
    {
        if(stream_tag_ == 0)
        {
            return IStream::has_unread_data();
        }else{
            bool ret = false;
            ((std::holds_alternative<S>(stream_var_) && ret = std::get<S>(stream_var_).has_unread_data()),...);
            return ret;
        }
    }
    MQAS_STREAM_IMPL_TEMPLATE_DECL
    std::span<const uint8_t> StreamVariant<S...>::read(size_t sz)
    {
        if(stream_tag_ == 0)
        {
            return IStream::read(sz);
        }else{
            std::span<const uint8_t> ret;
            ((std::holds_alternative<S>(stream_var_) && (ret = std::get<S>(stream_var_).read(sz),false)),...);
            return ret;
        }
    }
    MQAS_STREAM_IMPL_TEMPLATE_DECL
    std::span<const uint8_t> StreamVariant<S...>::read_all()
    {
        if(stream_tag_ == 0)
        {
            return IStream::read_all();
        }else{
            std::span<const uint8_t> ret;
            ((std::holds_alternative<S>(stream_var_) && (ret = std::get<S>(stream_var_).read_all()),false),...);
            return ret;
        }
    }
    MQAS_STREAM_IMPL_TEMPLATE_DECL
    std::span<const uint8_t> StreamVariant<S...>::read_all_not_move() const
    {
        if(stream_tag_ == 0)
        {
            return IStream::read_all_not_move();
        }else{
            std::span<const uint8_t> ret;
            ((std::holds_alternative<S>(stream_var_) && (ret = std::get<S>(stream_var_).read_all_not_move()),false),...);
            return ret;
        }
    }
    MQAS_STREAM_IMPL_TEMPLATE_DECL
    size_t  StreamVariant<S...>::unread_size() const
    {
        if(stream_tag_ == 0)
        {
            return IStream::unread_size();
        }else{
            size_t ret;
            ((std::holds_alternative<S>(stream_var_) && ret = std::get<S>(stream_var_).unread_size()),...);
            return ret;
        }
    }
    MQAS_STREAM_IMPL_TEMPLATE_DECL
    size_t StreamVariant<S...>::on_read(const std::span<const uint8_t>& current)
    {
        auto [msg,use_len] = stream_variant_msg::parse_command(current);
        if(!msg) return use_len;
        switch (msg->cmd) {
            case stream_variant_cmd::req_use_stream_tag:
                if(msg->param3 == 1) // is ack
                {
                    if(msg->errcode == StreamVariantErrcode::ok)
                    {
                        if(auto ret = change_to(static_cast<size_t>(msg->param1),msg->extra_params);ret != StreamVariantErrcode::ok)
                            LOG(ERROR) << "StreamVariant handle ack change to " << msg->param1 << " failed error = " << (int)ret;
                    }else
                        LOG(ERROR) << "StreamVariant change to " << msg->param1 << " failed error = " << (int)msg->errcode;
                }else // is req
                {
                    msg->errcode = StreamVariantErrcode::ok;
                    msg->param3 = 1;
                    if(auto ret = change_to(static_cast<size_t>(msg->param1),msg->extra_params); ret != StreamVariantErrcode::ok) {
                        LOG(ERROR) << "StreamVariant handle req change to " << msg->param1 << " failed error = " << (int)ret;
                        msg->errcode = ret;
                    }
                    auto data = msg->generate();
                    write({data->begin(),data->end()});
                }
                break;
        }
        return use_len;
    }
    MQAS_STREAM_IMPL_TEMPLATE_DECL
    StreamVariantErrcode StreamVariant<S...>::change_to(size_t tag,const std::span<uint8_t>& change_params)
    {
        StreamVariantErrcode ret = StreamVariantErrcode::failed_not_find;
        ((S::STREAM_TAG == tag && ((ret = change_to_uncheck<S>(change_params)), false)),...);
        return ret;
    }
    MQAS_STREAM_IMPL_TEMPLATE_DECL
    void StreamVariant<S...>::clear_curr_stream()
    {
        if(stream_tag_ != 0)
        {
            stream_tag_ = 0;
            on_close();
            stream_var_ = std::monostate{};
        }
    }
}

#undef MQAS_STREAM_IMPL_TEMPLATE_DECL
#endif //MQAS_STREAM_IMPL_HPP

#pragma clang diagnostic pop