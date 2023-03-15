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
            return do_read_shell();
        }else{
            return do_read_hold();
        }
    }
    MQAS_STREAM_IMPL_TEMPLATE_DECL
    size_t StreamVariant<S...>::do_read_shell()
    {
        auto ret = IStream::do_read();
        if(ret > 0) {
            const auto span = IStream::read_all_not_move();
            if (const auto read_len = on_read(span); read_len > 0)
                IStream::move_read_pos_uncheck(read_len);
        }
        return ret;
    }
    MQAS_STREAM_IMPL_TEMPLATE_DECL
    size_t StreamVariant<S...>::do_read_hold()
    {
        size_t ret = 0;
        ((std::holds_alternative<S>(stream_var_) && (ret = do_read_curr(std::get<S>(stream_var_)))),...);
        return ret;
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
    void StreamVariant<S...>::on_peer_change_ret(StreamVariantErrcode code,const std::span<uint8_t>& params)
    {
        if(stream_tag_ > 0)
        {
            ((std::holds_alternative<S>(stream_var_) && (std::get<S>(stream_var_).on_peer_change_ret(code,params),false)),...);
        }
    }
    MQAS_STREAM_IMPL_TEMPLATE_DECL
    StreamVariantErrcode StreamVariant<S...>::on_peer_quit(const std::span<uint8_t> &d,
                                           std::array<uint8_t, stream_variant_msg::EXTRA_PARAMS_MAX_SIZE>& buf,
                                           size_t& sz)
    {
        if(stream_tag_ > 0)
        {
            StreamVariantErrcode ret = StreamVariantErrcode::ok;
            ((std::holds_alternative<S>(stream_var_) && (ret = std::get<S>(stream_var_).on_peer_quit(d,buf,sz),false)),...);
            return ret;
        }
        return StreamVariantErrcode::failed_not_find;
    }
    MQAS_STREAM_IMPL_TEMPLATE_DECL
    void StreamVariant<S...>::on_peer_quit_ret(StreamVariantErrcode e,const std::span<uint8_t>& d)
    {
        if(stream_tag_ > 0)
        {
            ((std::holds_alternative<S>(stream_var_) && (std::get<S>(stream_var_).on_peer_quit_ret(e,d),false)),...);
        }
    }
    MQAS_STREAM_IMPL_TEMPLATE_DECL
    bool StreamVariant<S...>::req_quit(uint32_t curr_tag,const std::span<uint8_t> &d)
    {
        if(stream_tag_ > 0)
        {
            bool ret = false;
            ((std::holds_alternative<S>(stream_var_) && ret = std::get<S>(stream_var_).req_quit(d)),...);
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
                    if(msg->errcode != StreamVariantErrcode::ok)
                        LOG(ERROR) << "StreamVariant peer change to " << msg->param1 << " failed error = " << (int)msg->errcode;
                    on_peer_change_ret(msg->errcode,msg->extra_params);
                }else // is req
                {
                    std::array<uint8_t,stream_variant_msg::EXTRA_PARAMS_MAX_SIZE> ret_buf{};
                    size_t buf_len = 0;
                    msg->errcode = StreamVariantErrcode::ok;
                    msg->param3 = 1;
                    if(auto ret = change_to(static_cast<size_t>(msg->param1),msg->extra_params,ret_buf,buf_len); ret != StreamVariantErrcode::ok) {
                        LOG(ERROR) << "StreamVariant handle req change to " << msg->param1 << " failed error = " << (int)ret;
                        msg->errcode = ret;
                    }
                    if(buf_len > 0)
                        msg->extra_params = std::span<uint8_t >({&ret_buf[0],buf_len});
                    else
                        msg->extra_params = {};
                    auto data = msg->generate();
                    write({data->begin(),data->end()});
                }
                break;
            case stream_variant_cmd::req_quit_hold_stream:
                if(msg->param3 == 1) // is ack
                {
                    on_peer_quit_ret(msg->errcode,msg->extra_params);
                    if(msg->errcode != StreamVariantErrcode::ok)
                        LOG(ERROR) << "StreamVariant peer quit hold stream failed error = " << (int)msg->errcode;
                    else
                        clear_curr_stream();
                }else{
                    std::array<uint8_t,stream_variant_msg::EXTRA_PARAMS_MAX_SIZE> ret_buf{};
                    size_t buf_len = 0;
                    msg->param3 = 1;
                    msg->errcode = StreamVariantErrcode::ok;
                    if(msg->param1 != stream_tag_)
                        msg->errcode = StreamVariantErrcode::tag_not_eq;
                    if(msg->errcode == StreamVariantErrcode::ok) {
                        msg->errcode = on_peer_quit(msg->extra_params,ret_buf,buf_len);
                        clear_curr_stream();
                    }
                    if(buf_len > 0)
                        msg->extra_params = std::span<uint8_t >({&ret_buf[0],buf_len});
                    else
                        msg->extra_params = {};
                    auto data = msg->generate();
                    write({data->begin(),data->end()});
                }
                break;
        }
        return use_len;
    }
    MQAS_STREAM_IMPL_TEMPLATE_DECL
    StreamVariantErrcode StreamVariant<S...>::change_to(size_t tag,const std::span<uint8_t>& change_params,
                                                        std::array<uint8_t,stream_variant_msg::EXTRA_PARAMS_MAX_SIZE>& ret_buf,
                                                        size_t& buf_len)
    {
        StreamVariantErrcode ret = StreamVariantErrcode::failed_not_find;
        ((S::STREAM_TAG == tag && ((ret = change_to_uncheck<S>(change_params,ret_buf,buf_len)), false)),...);
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
    MQAS_STREAM_IMPL_TEMPLATE_DECL
    template<typename CS>
    requires variability_stream_require<CS>
    bool StreamVariant<S...>::req_change(const std::span<uint8_t>& change_params)
    {
        return req_change_to<CS,S...>(change_params);
    }

    MQAS_STREAM_IMPL_TEMPLATE_DECL
    template<typename CS>
    requires variability_stream_require<CS>
    StreamVariantErrcode StreamVariant<S...>::change_self(const std::span<uint8_t>& change_params,
                                     std::array<uint8_t,stream_variant_msg::EXTRA_PARAMS_MAX_SIZE>& ret_buf,
                                     size_t& buf_len)
    {
        return change_self_inside<CS,S...>(change_params,ret_buf,buf_len);
    }

    MQAS_STREAM_IMPL_TEMPLATE_DECL
    template<class CS>
    requires variability_stream_require<CS>
    CS* StreamVariant<S...>::get_holds_stream()
    {
        if(std::holds_alternative<CS>(stream_var_))
        {
            return &(std::get<CS>(stream_var_));
        }
        return nullptr;
    }
    MQAS_STREAM_IMPL_TEMPLATE_DECL
    bool StreamVariant<S...>::has_holds_stream() const
    {
        return !std::holds_alternative<std::monostate>(stream_var_);
    }
    MQAS_STREAM_IMPL_TEMPLATE_DECL
    template<typename CS>
    requires (std::is_base_of_v<IStream,CS>)
    size_t StreamVariant<S...>::do_read_curr(CS& cs)
    {
        if(cs.isWaitPeerChangeRet())
            return do_read_shell();
        const auto ret = cs.do_read();
        if(ret > 0) {
            const auto span = cs.read_all_not_move();
            if (const size_t read_len = cs.on_read(span);read_len > 0)
                cs.move_read_pos_uncheck(read_len);
        }
        return ret;
    }

    MQAS_STREAM_IMPL_TEMPLATE_DECL
    template<typename CS, typename F,typename ... Ss>
    requires requires{
        requires variability_stream_require<CS> && variability_stream_require<F>;
        requires (variability_stream_require<Ss> && ...);
    }
    StreamVariantErrcode StreamVariant<S...>::change_self_inside(const std::span<uint8_t>& change_params,
                                            std::array<uint8_t,stream_variant_msg::EXTRA_PARAMS_MAX_SIZE>& ret_buf,
                                            size_t& buf_len)
    {
        if constexpr(std::is_same_v<F,CS>)
        {
            return change_to_uncheck<CS>(change_params,ret_buf,buf_len);
        }else{
            return change_self_inside<CS,Ss...>(change_params,ret_buf,buf_len);
        }
    }

    MQAS_STREAM_IMPL_TEMPLATE_DECL
    template<typename CS>
    requires variability_stream_require<CS>
    StreamVariantErrcode StreamVariant<S...>::change_self_inside([[maybe_unused]] const std::span<uint8_t>& change_params)
    {
        return StreamVariantErrcode::failed_not_find;
    }

    MQAS_STREAM_IMPL_TEMPLATE_DECL
    template<typename CS>
    requires variability_stream_require<CS>
    StreamVariantErrcode StreamVariant<S...>::change_to_uncheck(const std::span<uint8_t>& change_params,
                                           std::array<uint8_t,stream_variant_msg::EXTRA_PARAMS_MAX_SIZE>& ret_buf,
                                           size_t& buf_len,bool is_req)
    {
        clear_curr_stream();
        stream_tag_ = CS::STREAM_TAG;
        stream_var_ = CS{};
        auto& stream = std::get<CS>(stream_var_);
        StreamVariantErrcode res = change_params.empty() ? stream.on_change(ret_buf,buf_len) :
                                   stream.on_change_with_params(change_params,ret_buf,buf_len);
        if(res != StreamVariantErrcode::ok) {
            clear_curr_stream();
            return res;
        }
        if(is_req)stream.setIsWaitPeerChangeRet(true);
        stream.set_cxt(cxt_);
        stream.on_init(stream_,connect_cxt_);
        return StreamVariantErrcode::ok;
    }

    MQAS_STREAM_IMPL_TEMPLATE_DECL
    template<typename CS, typename F,typename ... Ss>
    requires requires{
        requires variability_stream_require<CS> && variability_stream_require<F>;
        requires (variability_stream_require<Ss> && ...);
    }
    bool StreamVariant<S...>::req_change_to(const std::span<uint8_t>& change_params)
    {
        if(change_params.size() > stream_variant_msg::EXTRA_PARAMS_MAX_SIZE)
            return false;
        if constexpr(std::is_same_v<F,CS>)
        {
            std::array<uint8_t,stream_variant_msg::EXTRA_PARAMS_MAX_SIZE> ret_buf{};
            std::size_t buf_len = 0;
            auto ret = change_to_uncheck<CS>(change_params,ret_buf,buf_len,true);
            if(ret != StreamVariantErrcode::ok)
            {
                LOG(ERROR) << "req_change_to " << CS::STREAM_TAG << " change self failed error = " << (size_t)ret;
                return false;
            }
            stream_variant_msg msg{};
            msg.cmd = stream_variant_cmd::req_use_stream_tag;
            msg.param1 = static_cast<uint32_t >(CS::STREAM_TAG);
            if(buf_len > 0)
                msg.extra_params = std::span<uint8_t>(&ret_buf[0],buf_len);
            auto data = msg.generate();
            return write({*data});
        }else{
            return req_change_to<CS,Ss...>(change_params);
        }
    }

    MQAS_STREAM_IMPL_TEMPLATE_DECL
    template<typename CS>
    requires variability_stream_require<CS>
    bool StreamVariant<S...>::req_change_to([[maybe_unused]] const std::span<uint8_t>& change_params)
    {
        return false;
    }
    MQAS_STREAM_IMPL_TEMPLATE_DECL
    template<typename CS,typename MP>
    requires variability_stream_require<CS> && IsProtoBufMsgConf<MP>
    bool StreamVariant<S...>::req_change_to(const typename MP::PB_MSG_TYPE& m)
    {
        std::vector<uint8_t> buf(m.ByteSizeLong());
        if(!m.SerializePartialToArray(buf.data(),buf.size()))
            return false;
        return req_change_to<CS,S...>({buf});
    }
}

#undef MQAS_STREAM_IMPL_TEMPLATE_DECL
#endif //MQAS_STREAM_IMPL_HPP

#pragma clang diagnostic pop