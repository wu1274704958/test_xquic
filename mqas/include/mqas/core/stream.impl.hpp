//
// Created by Administrator on 2023/3/1.
//

#ifndef MQAS_STREAM_IMPL_HPP
#define MQAS_STREAM_IMPL_HPP
#ifdef __clang__
#pragma clang diagnostic push
#pragma ide diagnostic ignored "HidingNonVirtualFunction"
#pragma clang diagnostic ignored "-Wunused-value"
#endif

#include <assert.h>


#define MQAS_STREAM_IMPL_TEMPLATE_DECL                                  \
template<typename ... S>                                                \
requires requires{                                                      \
    requires (variability_stream_pair_require<S> && ...);                    \
}


template<typename T, typename = std::void_t<>>
struct has_member_function_on_resume : std::false_type {};

template<typename T>
struct has_member_function_on_resume<T,std::void_t<decltype(std::declval<T>().on_resume())>>
    : std::true_type {};

template<typename T, typename = std::void_t<>>
struct has_member_function_on_pause : std::false_type {};

template<typename T>
struct has_member_function_on_pause<T, std::void_t<decltype(std::declval<T>().on_pause())>>
    : std::true_type {};

namespace mqas::core{
    MQAS_STREAM_IMPL_TEMPLATE_DECL
    size_t StreamVariant<S...>::do_read() {
        if(_stream_tag == 0)
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
        while(IStream::has_unread_data()) {
            const auto span = IStream::read_all_not_move();
            if (const auto read_len = on_read(span); read_len > 0)
                IStream::move_read_pos_uncheck(read_len);
            else if(read_len == 0)
                break;
        }
        return ret;
    }
    MQAS_STREAM_IMPL_TEMPLATE_DECL
    size_t StreamVariant<S...>::do_read_hold()
    {
        size_t ret = 0;
        ((std::holds_alternative<std::shared_ptr<typename S::STREAM_TYPE>>(_stream_var) && (ret = do_read_curr(*std::get<std::shared_ptr<typename S::STREAM_TYPE>>(_stream_var)))),...);
        return ret;
    }
    MQAS_STREAM_IMPL_TEMPLATE_DECL
    void StreamVariant<S...>::do_write()
    {
        if(_stream_tag == 0)
        {
            IStream::do_write();
        }else{
            ((std::holds_alternative<std::shared_ptr<typename S::STREAM_TYPE>>(_stream_var) && (std::get<std::shared_ptr<typename S::STREAM_TYPE>>(_stream_var)->do_write(),false)),...);
        }
    }
    MQAS_STREAM_IMPL_TEMPLATE_DECL
    void StreamVariant<S...>::on_close()
    {
        on_close_signal.emit(get_holds_stream(_stream_tag));
        if(_stream_tag == 0)
        {
            IStream::on_close();
        }else{
            ((std::holds_alternative<std::shared_ptr<typename S::STREAM_TYPE>>(_stream_var) && (std::get<std::shared_ptr<typename S::STREAM_TYPE>>(_stream_var)->on_close(),false)),...);
        }
    }
    MQAS_STREAM_IMPL_TEMPLATE_DECL
    void StreamVariant<S...>::on_reset(StreamAspect how)
    {
        if(_stream_tag == 0)
        {
            IStream::on_reset(how);
        }else{
            ((std::holds_alternative<std::shared_ptr<typename S::STREAM_TYPE>>(_stream_var) && (std::get<std::shared_ptr<typename S::STREAM_TYPE>>(_stream_var)->on_reset(how),false)),...);
        }
    }
    MQAS_STREAM_IMPL_TEMPLATE_DECL
    template <bool Lazy>
    bool StreamVariant<S...>::write(const std::span<uint8_t>& d)
    {
        if(_stream_tag == 0)
        {
            if constexpr(Lazy)
            { 
                IStream::write_lazy(d);
                return true;
            }
            else {
                return IStream::write(d);
            }
        }else{
            bool ret = false;
            if constexpr (Lazy)
            {
                ((std::holds_alternative<std::shared_ptr<typename S::STREAM_TYPE>>(_stream_var) && (ret = (std::get<std::shared_ptr<typename S::STREAM_TYPE>>(_stream_var)->write_lazy(d),true))), ...);
            }
            else {
                ((std::holds_alternative<std::shared_ptr<typename S::STREAM_TYPE>>(_stream_var) && (ret = std::get<std::shared_ptr<typename S::STREAM_TYPE>>(_stream_var)->write(d))),...);
            }
            return ret;
        }
    }
    MQAS_STREAM_IMPL_TEMPLATE_DECL
    bool StreamVariant<S...>::want_read(bool f) const
    {
        if(_stream_tag == 0)
        {
            return IStream::want_read(f);
        }else{
            bool ret = false;
            ((std::holds_alternative<std::shared_ptr<typename S::STREAM_TYPE>>(_stream_var) && ret = std::get<std::shared_ptr<typename S::STREAM_TYPE>>(_stream_var)->want_read(f)),...);
            return ret;
        }
    }
    MQAS_STREAM_IMPL_TEMPLATE_DECL
    bool StreamVariant<S...>::want_write(bool f) const
    {
        if(_stream_tag == 0)
        {
            return IStream::want_write(f);
        }else{
            bool ret = false;
            ((std::holds_alternative<std::shared_ptr<typename S::STREAM_TYPE>>(_stream_var) && ret = std::get<std::shared_ptr<typename S::STREAM_TYPE>>(_stream_var)->want_write(f)),...);
            return ret;
        }
    }

    MQAS_STREAM_IMPL_TEMPLATE_DECL
    void* StreamVariant<S...>::get_cxt() const
    {
        if(_stream_tag == 0)
        {
            return IStream::get_cxt();
        }else{
            void* ret = nullptr;
            ((std::holds_alternative<std::shared_ptr<typename S::STREAM_TYPE>>(_stream_var) && (ret = std::get<std::shared_ptr<typename S::STREAM_TYPE>>(_stream_var)->get_cxt(),false)),...);
            return ret;
        }
    }
    MQAS_STREAM_IMPL_TEMPLATE_DECL
    void StreamVariant<S...>::set_cxt(void* c)
    {
        if(_stream_tag == 0)
        {
            IStream::set_cxt(c);
        }else{
            ((std::holds_alternative<std::shared_ptr<typename S::STREAM_TYPE>>(_stream_var) && (std::get<std::shared_ptr<typename S::STREAM_TYPE>>(_stream_var)->set_cxt(c),false)),...);
        }
    }
    MQAS_STREAM_IMPL_TEMPLATE_DECL
    void StreamVariant<S...>::clear_read_buf()
    {
        if(_stream_tag == 0)
        {
            IStream::clear_read_buf();
        }else{
            ((std::holds_alternative<std::shared_ptr<typename S::STREAM_TYPE>>(_stream_var) && (std::get<std::shared_ptr<typename S::STREAM_TYPE>>(_stream_var)->clear_read_buf(),false)),...);
        }
    }
    MQAS_STREAM_IMPL_TEMPLATE_DECL
    bool StreamVariant<S...>::has_unread_data() const
    {
        if(_stream_tag == 0)
        {
            return IStream::has_unread_data();
        }else{
            bool ret = false;
            ((std::holds_alternative<std::shared_ptr<typename S::STREAM_TYPE>>(_stream_var) && ret = std::get<std::shared_ptr<typename S::STREAM_TYPE>>(_stream_var)->has_unread_data()),...);
            return ret;
        }
    }
    MQAS_STREAM_IMPL_TEMPLATE_DECL
    std::span<const uint8_t> StreamVariant<S...>::read(size_t sz)
    {
        if(_stream_tag == 0)
        {
            return IStream::read(sz);
        }else{
            std::span<const uint8_t> ret;
            ((std::holds_alternative<std::shared_ptr<typename S::STREAM_TYPE>>(_stream_var) && (ret = std::get<std::shared_ptr<typename S::STREAM_TYPE>>(_stream_var)->read(sz),false)),...);
            return ret;
        }
    }
    MQAS_STREAM_IMPL_TEMPLATE_DECL
    std::span<const uint8_t> StreamVariant<S...>::read_all()
    {
        if(_stream_tag == 0)
        {
            return IStream::read_all();
        }else{
            std::span<const uint8_t> ret;
            ((std::holds_alternative<std::shared_ptr<typename S::STREAM_TYPE>>(_stream_var) && (ret = std::get<std::shared_ptr<typename S::STREAM_TYPE>>(_stream_var)->read_all()),false),...);
            return ret;
        }
    }
    MQAS_STREAM_IMPL_TEMPLATE_DECL
    std::span<const uint8_t> StreamVariant<S...>::read_all_not_move() const
    {
        if(_stream_tag == 0)
        {
            return IStream::read_all_not_move();
        }else{
            std::span<const uint8_t> ret;
            ((std::holds_alternative<std::shared_ptr<typename S::STREAM_TYPE>>(_stream_var) && (ret = std::get<std::shared_ptr<typename S::STREAM_TYPE>>(_stream_var)->read_all_not_move()),false),...);
            return ret;
        }
    }
    MQAS_STREAM_IMPL_TEMPLATE_DECL
    size_t  StreamVariant<S...>::unread_size() const
    {
        if(_stream_tag == 0)
        {
            return IStream::unread_size();
        }else{
            size_t ret;
            ((std::holds_alternative<std::shared_ptr<typename S::STREAM_TYPE>>(_stream_var) && ret = std::get<std::shared_ptr<typename S::STREAM_TYPE>>(_stream_var)->unread_size()),...);
            return ret;
        }
    }
    MQAS_STREAM_IMPL_TEMPLATE_DECL
    void StreamVariant<S...>::on_peer_change_ack(StreamVariantErrcode code,const std::span<uint8_t>& params)
    {
        if(_stream_tag > 0)
        {
            ((std::holds_alternative<std::shared_ptr<typename S::STREAM_TYPE>>(_stream_var) && (std::get<std::shared_ptr<typename S::STREAM_TYPE>>(_stream_var)->on_peer_change_ack(code,params),false)),...);
        }
    }
    MQAS_STREAM_IMPL_TEMPLATE_DECL
    StreamVariantErrcode StreamVariant<S...>::on_peer_quit(const std::span<uint8_t> &d,
                                           std::vector<uint8_t>& buf)
    {
        if(_stream_tag > 0)
        {
            StreamVariantErrcode ret = StreamVariantErrcode::ok;
            ((std::holds_alternative<std::shared_ptr<typename S::STREAM_TYPE>>(_stream_var) && (ret = std::get<std::shared_ptr<typename S::STREAM_TYPE>>(_stream_var)->on_peer_quit(d,buf),false)),...);
            return ret;
        }
        return StreamVariantErrcode::failed_not_find;
    }
    MQAS_STREAM_IMPL_TEMPLATE_DECL
    void StreamVariant<S...>::on_peer_quit_ack(StreamVariantErrcode e,const std::span<uint8_t>& d)
    {
        if(_stream_tag > 0)
        {
            ((std::holds_alternative<std::shared_ptr<typename S::STREAM_TYPE>>(_stream_var) && (std::get<std::shared_ptr<typename S::STREAM_TYPE>>(_stream_var)->on_peer_quit_ack(e,d),false)),...);
        }
    }
    MQAS_STREAM_IMPL_TEMPLATE_DECL
    bool StreamVariant<S...>::req_quit(const std::span<uint8_t> &d,bool lazy)
    {
        bool ret = false;
        if(_stream_tag > 0)
        {
            ((std::holds_alternative<std::shared_ptr<typename S::STREAM_TYPE>>(_stream_var) && (ret = std::get<std::shared_ptr<typename S::STREAM_TYPE>>(_stream_var)->req_quit(d,lazy),false)),...);
        }
        return ret;
    }
    MQAS_STREAM_IMPL_TEMPLATE_DECL
    bool StreamVariant<S...>::isWaitingPeerChangeAck() const
    {
        bool ret = false;
        if(_stream_tag > 0)
        {
            ((std::holds_alternative<std::shared_ptr<typename S::STREAM_TYPE>>(_stream_var) && ret = std::get<std::shared_ptr<typename S::STREAM_TYPE>>(_stream_var)->isWaitingPeerChangeAck()),...);
        }
        return ret;
    }
    MQAS_STREAM_IMPL_TEMPLATE_DECL
    void StreamVariant<S...>::setWaitingPeerChangeAck(bool v)
    {
        if(_stream_tag > 0)
        {
            ((std::holds_alternative<std::shared_ptr<typename S::STREAM_TYPE>>(_stream_var) && (std::get<std::shared_ptr<typename S::STREAM_TYPE>>(_stream_var)->setWaitingPeerChangeAck(v),false)),...);
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
                    assert(_current_state == variant_stream_state::req_wait_ack);
                    if (msg->errcode != StreamVariantErrcode::ok)
                    {
                        LOG(ERROR) << "StreamVariant peer change to " << msg->param1 << " failed error = " << (int)msg->errcode;
                        _current_state = variant_stream_state::none;
                        clear_curr_stream();
                    }
                    else {
                        _current_state = variant_stream_state::active;
                        setWaitingPeerChangeAck(false);
                    }
                    on_peer_change_ack(msg->errcode,msg->extra_params);
                }else // is req
                {
                    std::vector<uint8_t> ret_buf{};
                    msg->errcode = StreamVariantErrcode::ok;
                    msg->param3 = 1;
                    StreamVariantErrcode change_ret = try_push_curr_stream();
                    if (change_ret != StreamVariantErrcode::ok)
                    {
                        LOG(ERROR) << "req_change_to by peer " << msg->param1 << " change self failed error = " << (size_t)change_ret;
                        msg->errcode = change_ret;
                    }else{
                        change_ret = change_to(static_cast<size_t>(msg->param1),msg->extra_params,ret_buf);
                        if(change_ret != StreamVariantErrcode::ok && change_ret != StreamVariantErrcode::skip_and_manual) {
                            LOG(ERROR) << "StreamVariant handle req change to " << msg->param1 << " failed error = " << (int)change_ret;
                            msg->errcode = change_ret;
                        }
                        _current_state = variant_stream_state::active;
                        if (change_ret == StreamVariantErrcode::skip_and_manual)
                        {
                            _current_state = variant_stream_state::half_active;
                            break;
                        }
                    }
                    if(!ret_buf.empty())
                        msg->extra_params = std::span<uint8_t >({ret_buf});
                    else
                        msg->extra_params = {};
                    auto data = msg->generate();
                    write({data->begin(),data->end()});
                }
                break;
            case stream_variant_cmd::req_quit_hold_stream:
                if(msg->param3 == 1) // is ack
                {
                    assert(_current_state == variant_stream_state::quit_wait_ack);
                    on_peer_quit_ack(msg->errcode,msg->extra_params);
                    if (msg->errcode != StreamVariantErrcode::ok)
                    {
                        _current_state = variant_stream_state::active;
                        LOG(ERROR) << "StreamVariant peer quit hold stream failed error = " << (int)msg->errcode;
                    }
                    else
                    {
                        if (has_holds_stream())
                            on_quit_stream_signal.emit(get_holds_stream(_stream_tag));
                        _current_state = variant_stream_state::none;
                        clear_curr_stream();
                    }
                }else{
                    std::vector<uint8_t> ret_buf{};
                    msg->param3 = 1;
                    msg->errcode = StreamVariantErrcode::ok;
                    if(msg->param1 != _stream_tag)
                        msg->errcode = StreamVariantErrcode::tag_not_eq;
                    _current_state = variant_stream_state::none;
                    if(msg->errcode == StreamVariantErrcode::ok) {
                        if ((msg->errcode = on_peer_quit(msg->extra_params, ret_buf)) == StreamVariantErrcode::skip_and_manual)
                        {
                            _current_state = variant_stream_state::half_quit;
                            break;
                        }
                        if (has_holds_stream())
                            on_quit_stream_signal.emit(get_holds_stream(_stream_tag));
                        clear_curr_stream();
                    }
                    if(!ret_buf.empty())
                        msg->extra_params = {ret_buf};
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
                                                        std::vector<uint8_t>& ret_buf)
    {
        StreamVariantErrcode ret = StreamVariantErrcode::failed_not_find;
        ((S::STREAM_TAG == tag && ((ret = change_to_uncheck<S,false>(change_params,ret_buf)), false)),...);
        return ret;
    }
    MQAS_STREAM_IMPL_TEMPLATE_DECL
    void StreamVariant<S...>::clear_curr_stream()
    {
        if (!try_pop_stream())
        {
            if (_stream_tag != 0)
            {
                _stream_tag = 0;
                _stream_var = std::monostate{};
                _current_state = variant_stream_state::none;
            }
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
    template<typename CS,typename MP>
    requires variability_stream_require<CS> && IsProtoBufMsgConf<MP>
    bool StreamVariant<S...>::req_change(const typename MP::PB_MSG_TYPE& m)
    {
        auto buf = ProtoBufMsg::write_msg<MP>(m);
        if(!buf)
            return false;
        return req_change_to<CS,S...>({*buf});
    }
    MQAS_STREAM_IMPL_TEMPLATE_DECL
    template<typename CS>
    requires variability_stream_require<CS>
    StreamVariantErrcode StreamVariant<S...>::change_self(const std::span<uint8_t>& change_params,
                                     std::vector<uint8_t>& ret_buf)
    {
        return change_self_inside<CS,S...>(change_params,ret_buf);
    }

    MQAS_STREAM_IMPL_TEMPLATE_DECL
    template<class CS>
    requires variability_stream_require<CS>
    std::shared_ptr<CS> StreamVariant<S...>::get_holds_stream()
    {
        if(std::holds_alternative<std::shared_ptr<CS>>(_stream_var))
        {
            return std::get<std::shared_ptr<CS>>(_stream_var);
        }
        return nullptr;
    }

    MQAS_STREAM_IMPL_TEMPLATE_DECL
    std::shared_ptr<IStreamVariant> StreamVariant<S...>::get_holds_stream(size_t stream_tag)
    {
        std::shared_ptr<IStreamVariant> res = nullptr;
        if (stream_tag == 0)
            return res;
        ((std::holds_alternative<std::shared_ptr<typename S::STREAM_TYPE>>(_stream_var) && 
            (res = std::dynamic_pointer_cast<IStreamVariant>(std::get<std::shared_ptr<typename S::STREAM_TYPE>>(_stream_var)),false)), ...);
        return res;
    }

    MQAS_STREAM_IMPL_TEMPLATE_DECL
    bool StreamVariant<S...>::has_holds_stream() const
    {
        return !std::holds_alternative<std::monostate>(_stream_var);
    }
    MQAS_STREAM_IMPL_TEMPLATE_DECL
    template<typename CS>
    requires (std::is_base_of_v<IStream,CS>)
    size_t StreamVariant<S...>::do_read_curr(CS& cs)
    {
        if(cs.isWaitingPeerChangeAck())
            return do_read_shell();
        const auto ret = cs.do_read();
        while (cs.has_unread_data()) {
            const auto span = cs.read_all_not_move();
            if (const size_t read_len = cs.on_read(span);read_len > 0) {
                cs.move_read_pos_uncheck(read_len);
                if(cs.has_unread_data() && cs.isWaitingPeerChangeAck())
                {
                    hold_stream_unread_moveto_shell(cs);
                    return do_read_shell();
                }
            }else if(read_len == 0)
                break;
        }
        return ret;
    }
    MQAS_STREAM_IMPL_TEMPLATE_DECL
    template<typename CS>
    requires (std::is_base_of_v<IStream,CS>)
    void StreamVariant<S...>::hold_stream_unread_moveto_shell(CS& cs)
    {
        const auto span = cs.read_all();
        IStream::append_unread(std::span<uint8_t>((uint8_t*)span.data(),span.size()));
    }

    MQAS_STREAM_IMPL_TEMPLATE_DECL
    template<typename CS, typename F,typename ... Ss>
    requires requires{
        requires variability_stream_require<CS> && variability_stream_pair_require<F>;
        requires (variability_stream_pair_require<Ss> && ...);
    }
    StreamVariantErrcode StreamVariant<S...>::change_self_inside(const std::span<uint8_t>& change_params,
                                            std::vector<uint8_t>& ret_buf)
    {
        if constexpr(std::is_same_v<typename F::STREAM_TYPE,CS>)
        {
            return change_to_uncheck<F,true>(change_params,ret_buf);
        }else{
            return change_self_inside<CS,Ss...>(change_params,ret_buf);
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
    template<typename CS,bool IS_LOCAL>
    requires variability_stream_pair_require<CS>
    StreamVariantErrcode StreamVariant<S...>::change_to_uncheck(const std::span<uint8_t>& change_params,
                                           std::vector<uint8_t>& ret_buf)
    {
        //checked it earlier
        //clear_curr_stream();
        assert(_stream_tag == 0);
        _stream_tag = CS::STREAM_TAG;
        _stream_var = std::make_shared<typename CS::STREAM_TYPE>();
        auto stream = std::get<std::shared_ptr<typename CS::STREAM_TYPE>>(_stream_var);
        stream->setStreamTag(_stream_tag);
        stream->set_outer(this->weak_from_this());
        stream->set_cxt(cxt_);
        stream->on_init(_stream,connect_cxt_,connect);
        StreamVariantErrcode res;
        if constexpr (!IS_LOCAL)
        {
            on_change_stream_signal.emit(stream);
            res = stream->on_change(change_params, ret_buf);
            if (res != StreamVariantErrcode::ok && res != StreamVariantErrcode::skip_and_manual) {
                clear_curr_stream();
                return res;
            }
        }
        else {
            res = stream->on_local_change(change_params, ret_buf);//request local change can be not support
            if (res != StreamVariantErrcode::ok && res != StreamVariantErrcode::skip_and_manual && res != StreamVariantErrcode::not_support) {
                clear_curr_stream();
                return res;
            }
            stream->setWaitingPeerChangeAck(true);
        }
        return res;
    }

    MQAS_STREAM_IMPL_TEMPLATE_DECL
    template<typename CS, typename F,typename ... Ss>
    requires requires{
        requires variability_stream_require<CS> && variability_stream_pair_require<F>;
        requires (variability_stream_pair_require<Ss> && ...);
    }
    bool StreamVariant<S...>::req_change_to(const std::span<uint8_t>& change_params)
    {
        if(change_params.size() > stream_variant_msg::EXTRA_PARAMS_MAX_SIZE)
            return false;
        if constexpr(std::is_same_v<typename F::STREAM_TYPE,CS>)
        {
            auto ret = try_push_curr_stream();
            if (ret != StreamVariantErrcode::ok)
            {
                LOG(ERROR) << "req_change_to " << F::STREAM_TAG << " change self failed error = " << (size_t)ret;
                return false;
            }
            std::vector<uint8_t> ret_buf{}; 
            ret = change_to_uncheck<F,true>(change_params,ret_buf);                               //request change by self can be not support
            if(ret != StreamVariantErrcode::ok && ret != StreamVariantErrcode::skip_and_manual && ret != StreamVariantErrcode::not_support)
            {
                LOG(ERROR) << "req_change_to " << F::STREAM_TAG << " change self failed error = " << (size_t)ret;
                return false;
            }
            const auto use_input_data = ret == StreamVariantErrcode::not_support;
            _current_state = variant_stream_state::req_wait_ack;
            if (ret == StreamVariantErrcode::skip_and_manual)
            {
                _current_state = variant_stream_state::half_req;
                return true;
            }
            stream_variant_msg msg{};
            msg.cmd = stream_variant_cmd::req_use_stream_tag;
            msg.param1 = static_cast<uint32_t >(F::STREAM_TAG);
            if (!ret_buf.empty())
                msg.extra_params = { ret_buf };
            else if (use_input_data)
                msg.extra_params = change_params;
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
    StreamVariantErrcode StreamVariant<S...>::try_push_curr_stream()
    {
        switch (_current_state)
        {
            case variant_stream_state::active:
            case variant_stream_state::none:
            break;
            default:
            LOG(ERROR) << "try_push_curr_stream incorrect state current is " << (size_t)_current_state;
            return StreamVariantErrcode::incorrect_state;
        }
        if (_stream_tag == 0)
            return StreamVariantErrcode::ok;
        ((S::STREAM_TAG == _stream_tag && ((push_stream<S>(std::get<std::shared_ptr<typename S::STREAM_TYPE>>(_stream_var))), false)), ...);
        return StreamVariantErrcode::ok;
    }

    MQAS_STREAM_IMPL_TEMPLATE_DECL
    template<typename SP>
        requires variability_stream_pair_require<SP>
    void StreamVariant<S...>::push_stream(std::shared_ptr<typename SP::STREAM_TYPE> stream)
    {
        if constexpr (has_member_function_on_pause<typename SP::STREAM_TYPE>::value)
        {
            on_pause_stream_signal.emit(stream);
            stream->on_pause();
        }
        _stack.push(std::make_pair(SP::STREAM_TAG,std::dynamic_pointer_cast<IStreamVariant>(stream)));
        _stream_tag = 0;
        _stream_var = std::monostate{};
    }

    MQAS_STREAM_IMPL_TEMPLATE_DECL
    bool StreamVariant<S...>::try_pop_stream()
    {
        if(_stack.empty())
            return false;
        auto top = _stack.top();
        _stack.pop();
        ((S::STREAM_TAG == top.first && ((set_curr_stream<S>(top.second)),false)), ...);
        return true;
    }
    MQAS_STREAM_IMPL_TEMPLATE_DECL
    template<typename SP>
        requires variability_stream_pair_require<SP>
    void StreamVariant<S...>::set_curr_stream(std::shared_ptr<IStreamVariant> stream)
    {
        _stream_tag = SP::STREAM_TAG;
        _stream_var = std::dynamic_pointer_cast<typename SP::STREAM_TYPE>(stream);
        _current_state = variant_stream_state::active;

        if constexpr (has_member_function_on_resume<typename SP::STREAM_TYPE>::value)
        {
            std::dynamic_pointer_cast<typename SP::STREAM_TYPE>(stream)->on_resume();
            on_resume_stream_signal.emit(stream);
        }
    }

    MQAS_STREAM_IMPL_TEMPLATE_DECL
    void StreamVariant<S...>::on_req_quit()
    {
        _current_state = variant_stream_state::quit_wait_ack;
    }
    //-1 error; 0 success; 1 success but not write;2 write but failed
    MQAS_STREAM_IMPL_TEMPLATE_DECL
    int StreamVariant<S...>::on_send_sv_msg(const stream_variant_msg& msg,std::vector<uint8_t>& buf)
    {
        bool need_write = false;
        if (msg.param1 != _stream_tag)
            return -1;
        if (msg.cmd == core::stream_variant_cmd::req_use_stream_tag)
        {
            if (_current_state != variant_stream_state::half_req && _current_state != variant_stream_state::half_active)
                return -1;
        }
        if (msg.cmd == core::stream_variant_cmd::req_quit_hold_stream && _current_state != variant_stream_state::half_quit)
            return -1;
        bool is_ok = msg.errcode == StreamVariantErrcode::ok;
        switch (_current_state)
        {
        case variant_stream_state::half_active:
            _current_state = is_ok ? variant_stream_state::active : variant_stream_state::none;
            need_write = !is_ok;
            if (!is_ok)
                clear_curr_stream();
            break;
        case variant_stream_state::half_req:
            _current_state = is_ok ? variant_stream_state::req_wait_ack : variant_stream_state::none;
            need_write = !is_ok;
            if(!is_ok)
                clear_curr_stream();
            break;
        case variant_stream_state::half_quit:
            _current_state = is_ok ? variant_stream_state::none : variant_stream_state::active;
            need_write = is_ok;
            if(is_ok)
                clear_curr_stream();
            break;
        default:
            break;
        }
        if (need_write)
            return write({ buf }) ? 1 : 2;
        return need_write ? 1 : 0;
    }
}

#undef MQAS_STREAM_IMPL_TEMPLATE_DECL

#ifdef __clang__
#pragma clang diagnostic pop
#endif

#endif //MQAS_STREAM_IMPL_HPP