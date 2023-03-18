//
// Created by Administrator on 2023/3/10.
//

#pragma clang diagnostic push
#pragma ide diagnostic ignored "HidingNonVirtualFunction"
#ifndef MQAS_CORE_PB_STREAM_IMPL_HPP
#define MQAS_CORE_PB_STREAM_IMPL_HPP

#include <type_traits>

#define MQAS_PB_STREAM_TEMPLATE_DECL                        \
template<class S,class ... M>                                     \
requires requires{ requires (IsProtoBufMsgConf<M> && ...);}

namespace mqas::core {

    template<typename T,typename M, typename = std::void_t<>>
    struct has_member_function_on_read_msg_s : std::false_type {};

    template<typename T,typename M>
    struct has_member_function_on_read_msg_s<T,M,std::void_t<decltype(std::declval<T>().on_read_msg_s(std::declval<std::shared_ptr<M>>()))>>
            : std::true_type {};

    template<typename T,typename M,typename = std::void_t<>>
    struct has_member_function_on_change_msg_s : std::false_type {};

    template<typename T,typename M>
    struct has_member_function_on_change_msg_s<T,M,std::void_t<decltype(std::declval<T>().on_change_msg_s(std::declval<std::shared_ptr<M>>(),
            std::declval<std::vector<uint8_t>&>()))>>
            : std::true_type {};

    template<typename T,typename M,typename = std::void_t<>>
    struct has_member_function_on_peer_change_ret_msg_s : std::false_type {};

    template<typename T,typename M>
    struct has_member_function_on_peer_change_ret_msg_s<T,M,std::void_t<decltype(std::declval<T>().on_peer_change_ret_msg_s(std::declval<StreamVariantErrcode>(),
            std::declval<std::shared_ptr<M>>()))>>
            : std::true_type {};

    template<typename T,typename M,typename = std::void_t<>>
    struct has_member_function_on_peer_quit_ret_msg_s : std::false_type {};

    template<typename T,typename M>
    struct has_member_function_on_peer_quit_ret_msg_s<T,M,std::void_t<decltype(std::declval<T>().on_peer_quit_ret_msg_s(std::declval<StreamVariantErrcode>(),
            std::declval<std::shared_ptr<M>>()))>>
            : std::true_type {};

    template<typename T,typename M,typename = std::void_t<>>
    struct has_member_function_on_peer_quit_msg_s : std::false_type {};

    template<typename T,typename M>
    struct has_member_function_on_peer_quit_msg_s<T,M,std::void_t<decltype(std::declval<T>().on_peer_quit_msg_s(std::declval<std::shared_ptr<M>>(),
            std::declval<std::vector<uint8_t>&>()))>>
            : std::true_type {};

    MQAS_PB_STREAM_TEMPLATE_DECL
    void ProtoBufStream<S,M...>::on_init(::lsquic_stream_t* lsquic_stream,connect_cxt* connect_cxt)
    {
        IStream::on_init(lsquic_stream,connect_cxt);
        init_msg_parsers();
    }
    MQAS_PB_STREAM_TEMPLATE_DECL
    void ProtoBufStream<S,M...>::init_msg_parsers()
    {
        (init_msg_parser<M>(),...);
    }
    MQAS_PB_STREAM_TEMPLATE_DECL
    template<class CM>
    void ProtoBufStream<S,M...>::init_msg_parser() {
        using MSG_T = typename CM::PB_MSG_TYPE;
        auto pb_stream = this;
        auto [it,ok] = msg_parsers_.try_emplace(CM::PB_MSG_ID,[pb_stream](const proto::MsgWrapper& wrapper,MsgOrigin origin)->std::shared_ptr<google::protobuf::Message>{
            auto msg = std::make_shared<MSG_T>();
            if(!msg->ParsePartialFromString(wrapper.msg_body()))
            {
                LOG(ERROR) << "Try parse proto buf message " << CM::PB_MSG_ID << " failed";
                return nullptr;
            }
            auto base_msg = std::dynamic_pointer_cast<google::protobuf::Message>(msg);
            //forward msg by origin
            if(origin == MsgOrigin::normal) {
                if constexpr (has_member_function_on_read_msg_s<S,MSG_T>::value)
                {
                     static_cast<S*>(pb_stream)->on_read_msg_s(msg);
                }else{
                    static_cast<S*>(pb_stream)->on_read_msg(CM::PB_MSG_ID,base_msg);
                }
                if(pb_stream->msg_handlers_.contains(CM::PB_MSG_ID))
                    pb_stream->msg_handlers_[CM::PB_MSG_ID].emit(CM::PB_MSG_ID,base_msg);
            }
            return base_msg;
        });
        if(!ok)
            LOG(WARNING) << "Not insert proto buf message handler " << CM::PB_MSG_ID << " cause by already has one";
    }
    MQAS_PB_STREAM_TEMPLATE_DECL
    std::shared_ptr<proto::MsgWrapper> ProtoBufStream<S,M...>::parse_base_msg(const std::span<uint8_t>& d) const
    {
        auto msg = std::make_shared<proto::MsgWrapper>();
        if(!msg->ParsePartialFromArray(d.data(),d.size()))
        {
            if(msg->body_len() == msg->msg_body().size())
                return msg;
            //LOG(ERROR) << "Try parse proto::MsgWrapper failed";
            return nullptr;
        }
        return msg;
    }
    MQAS_PB_STREAM_TEMPLATE_DECL
    sigc::connection ProtoBufStream<S,M...>::add_handler(size_t mid,const HANDLER_FUN_TY::slot_type& f)
    {
        if(!msg_handlers_.contains(mid))
            msg_handlers_.emplace(mid);
        return msg_handlers_[mid].connect(f);
    }
    MQAS_PB_STREAM_TEMPLATE_DECL
    bool ProtoBufStream<S,M...>::has_handler(size_t mid)const
    {
        return msg_handlers_.contains(mid);
    }
    MQAS_PB_STREAM_TEMPLATE_DECL
    size_t ProtoBufStream<S,M...>::get_handlers_count(size_t mid) const
    {
        if(msg_handlers_.contains(mid))
            return msg_handlers_.at(mid).size();
        return 0;
    }
    MQAS_PB_STREAM_TEMPLATE_DECL
    void ProtoBufStream<S,M...>::erase_all_handlers(size_t mid)
    {
        if(msg_handlers_.contains(mid))
        {
            msg_handlers_[mid].clear();
        }
    }

    MQAS_PB_STREAM_TEMPLATE_DECL
    StreamVariantErrcode ProtoBufStream<S,M...>::on_change(const std::span<uint8_t> &params,
                                               std::vector<uint8_t> &ret_buf)
    {
        if(params.empty())
            return on_change_msg_forward<M...>(0, nullptr,ret_buf);
        auto msg_wrap = parse_base_msg(params);
        if(!msg_wrap) {
            LOG(ERROR) << "Try parse proto::MsgWrapper failed on_change";
            return StreamVariantErrcode::parse_failed;
        }
        size_t mid = msg_wrap->msg_id();
        if(!msg_parsers_.contains(mid)){
            LOG(ERROR) << "Not support msg "<< mid <<" on_change";
            return StreamVariantErrcode::not_support;
        }
        auto msg = msg_parsers_[mid](*msg_wrap,MsgOrigin::on_change);
        return on_change_msg_forward<M...>(mid,msg,ret_buf);
    }
    MQAS_PB_STREAM_TEMPLATE_DECL
    StreamVariantErrcode ProtoBufStream<S,M...>::on_change_msg(size_t,const std::shared_ptr<google::protobuf::Message>&,
                                               std::vector<uint8_t> &ret_buf)
    {
        return StreamVariantErrcode::not_support;
    }
    MQAS_PB_STREAM_TEMPLATE_DECL
    template<class F,class ... Ss>
    StreamVariantErrcode ProtoBufStream<S,M...>::on_change_msg_forward(size_t mid,const std::shared_ptr<google::protobuf::Message>& msg,
            std::vector<uint8_t> &ret_buf)
    {
        if(F::PB_MSG_ID == mid)
        {
            if constexpr (has_member_function_on_change_msg_s<S,typename F::PB_MSG_TYPE>::value)
            {
                return static_cast<S*>(this)->on_change_msg_s(std::static_pointer_cast<typename F::PB_MSG_TYPE>(msg),ret_buf);
            }else{
                return static_cast<S*>(this)->on_change_msg(mid,msg,ret_buf);
            }
        }else{
            if constexpr (sizeof...(Ss) == 0)
            {
                return static_cast<S*>(this)->on_change_msg(mid,msg,ret_buf);
            }else{
                return on_change_msg_forward<Ss...>(mid,msg,ret_buf);
            }
        }

    }

    MQAS_PB_STREAM_TEMPLATE_DECL
    void ProtoBufStream<S,M...>::on_peer_change_ret(StreamVariantErrcode code, const std::span<uint8_t> &params)
    {
        if(params.empty())
            return on_peer_change_ret_msg_forward<M...>(code,0, nullptr);
        auto msg_wrap = parse_base_msg(params);
        if(!msg_wrap) {
            LOG(ERROR) << "Try parse proto::MsgWrapper failed on_peer_change_ret";
            return;
        }
        size_t mid = msg_wrap->msg_id();
        if(!msg_parsers_.contains(mid)){
            LOG(ERROR) << "Not support msg "<< mid <<" on_peer_change_ret";
            return;
        }
        auto msg = msg_parsers_[mid](*msg_wrap,MsgOrigin::on_peer_change_ret);
        on_peer_change_ret_msg_forward<M...>(code,mid,msg);
    }
    MQAS_PB_STREAM_TEMPLATE_DECL
    void ProtoBufStream<S,M...>::on_peer_change_ret_msg(StreamVariantErrcode code, size_t,const std::shared_ptr<google::protobuf::Message>&){}
    MQAS_PB_STREAM_TEMPLATE_DECL
    template<class F,class ... Ss>
    void ProtoBufStream<S,M...>::on_peer_change_ret_msg_forward(StreamVariantErrcode code, size_t mid,const std::shared_ptr<google::protobuf::Message>& msg)
    {
        if(F::PB_MSG_ID == mid)
        {
            if constexpr (has_member_function_on_peer_change_ret_msg_s<S,typename F::PB_MSG_TYPE>::value)
            {
                static_cast<S*>(this)->on_peer_change_ret_msg_s(code,std::static_pointer_cast<typename F::PB_MSG_TYPE>(msg));
            }else{
                static_cast<S*>(this)->on_peer_change_ret_msg(code,mid,msg);
            }
        }else{
            if constexpr (sizeof...(Ss) == 0)
            {
                static_cast<S*>(this)->on_peer_change_ret_msg(code,mid,msg);
            }else{
                on_peer_change_ret_msg_forward<Ss...>(code,mid,msg);
            }
        }
    }

    MQAS_PB_STREAM_TEMPLATE_DECL
    size_t ProtoBufStream<S,M...>::on_read(const std::span<const uint8_t>& current)
    {
        auto msg_wrap = parse_base_msg(std::span<uint8_t>(const_cast<uint8_t*>(current.data()),current.size()));
        if(!msg_wrap) {
            LOG(ERROR) << "Try parse proto::MsgWrapper failed on_read";
            return 0;
        }
        size_t mid = msg_wrap->msg_id();
        if(!msg_parsers_.contains(mid)){
            LOG(ERROR) << "Not support msg "<< mid <<" on_read";
            return msg_wrap->ByteSizeLong();
        }
        auto msg = msg_parsers_[mid](*msg_wrap,MsgOrigin::normal);
        return msg_wrap->ByteSizeLong();
    }
    MQAS_PB_STREAM_TEMPLATE_DECL
    void ProtoBufStream<S,M...>::on_read_msg(size_t,const std::shared_ptr<google::protobuf::Message>&){}
    MQAS_PB_STREAM_TEMPLATE_DECL
    void ProtoBufStream<S,M...>::on_peer_quit_ret(StreamVariantErrcode e,const std::span<uint8_t>& d)
    {
        if(d.empty())
            return on_peer_quit_ret_msg_forward<M...>(e,0, nullptr);
        auto msg_wrap = parse_base_msg(d);
        if(!msg_wrap) {
            LOG(ERROR) << "Try parse proto::MsgWrapper failed on_peer_quit_ret";
            return;
        }
        size_t mid = msg_wrap->msg_id();
        if(!msg_parsers_.contains(mid)){
            LOG(ERROR) << "Not support msg "<< mid <<" on_peer_quit_ret";
            return ;
        }
        auto msg = msg_parsers_[mid](*msg_wrap,MsgOrigin::on_peer_quit_ret);
        return on_peer_quit_ret_msg_forward<M...>(e,mid,msg);
    }
    MQAS_PB_STREAM_TEMPLATE_DECL
    void ProtoBufStream<S,M...>::on_peer_quit_ret_msg(StreamVariantErrcode,size_t,const std::shared_ptr<google::protobuf::Message>&){}
    MQAS_PB_STREAM_TEMPLATE_DECL
    template<class F,class ... Ss>
    void ProtoBufStream<S,M...>::on_peer_quit_ret_msg_forward(StreamVariantErrcode e,size_t mid,const std::shared_ptr<google::protobuf::Message>& msg)
    {
        if(F::PB_MSG_ID == mid)
        {
            if constexpr (has_member_function_on_peer_quit_ret_msg_s<S,typename F::PB_MSG_TYPE>::value)
            {
                static_cast<S*>(this)->on_peer_quit_ret_msg_s(e,std::static_pointer_cast<typename F::PB_MSG_TYPE>(msg));
            }else{
                static_cast<S*>(this)->on_peer_quit_ret_msg(e,mid,msg);
            }
        }else{
            if constexpr (sizeof...(Ss) == 0)
            {
                static_cast<S*>(this)->on_peer_quit_ret_msg(e,mid,msg);
            }else{
                on_peer_quit_ret_msg_forward<Ss...>(e,mid,msg);
            }
        }
    }
    MQAS_PB_STREAM_TEMPLATE_DECL
    StreamVariantErrcode ProtoBufStream<S,M...>::on_peer_quit(const std::span<uint8_t> & d,
                                              std::vector<uint8_t>& ret_buf)
    {
        if(d.empty())
            return on_peer_quit_msg_forward<M...>(0, nullptr,ret_buf);
        auto msg_wrap = parse_base_msg(d);
        if(!msg_wrap) {
            LOG(ERROR) << "Try parse proto::MsgWrapper failed on_peer_quit";
            return  StreamVariantErrcode::parse_failed;
        }
        size_t mid = msg_wrap->msg_id();
        if(!msg_parsers_.contains(mid)){
            LOG(ERROR) << "Not support msg "<< mid <<" on_peer_quit";
            return  StreamVariantErrcode::not_support;
        }
        auto msg = msg_parsers_[mid](*msg_wrap,MsgOrigin::on_peer_quit);
        return on_peer_quit_msg_forward<M...>(mid, msg,ret_buf);
    }
    MQAS_PB_STREAM_TEMPLATE_DECL
    StreamVariantErrcode ProtoBufStream<S,M...>::on_peer_quit_msg(size_t,const std::shared_ptr<google::protobuf::Message>&,
                                              std::vector<uint8_t>&){
        return StreamVariantErrcode::not_support;
    }
    MQAS_PB_STREAM_TEMPLATE_DECL
    template<class F,class ... Ss>
    StreamVariantErrcode ProtoBufStream<S,M...>::on_peer_quit_msg_forward(size_t mid,const std::shared_ptr<google::protobuf::Message>& msg,
        std::vector<uint8_t>& ret_buf)
    {
        if(F::PB_MSG_ID == mid)
        {
            if constexpr (has_member_function_on_peer_quit_msg_s<S,typename F::PB_MSG_TYPE>::value)
            {
                return static_cast<S*>(this)->on_peer_quit_msg_s(std::static_pointer_cast<typename F::PB_MSG_TYPE>(msg),ret_buf);
            }else{
                return static_cast<S*>(this)->on_peer_quit_msg(mid,msg,ret_buf);
            }
        }else{
            if constexpr (sizeof...(Ss) == 0)
            {
                return static_cast<S*>(this)->on_peer_quit_msg(mid,msg,ret_buf);
            }else{
                return on_peer_quit_msg_forward<Ss...>(mid,msg,ret_buf);
            }
        }
    }
    MQAS_PB_STREAM_TEMPLATE_DECL
    template<class SM>
    requires IsProtoBufMsgConf<SM>
    bool ProtoBufStream<S,M...>::send(const typename SM::PB_MSG_TYPE& m)
    {
        auto buf = ProtoBufMsg::write_msg<SM>(m);
        if(!buf)
            return false;
        return write({*buf});
    }
    MQAS_PB_STREAM_TEMPLATE_DECL
    template<class SM>
    requires IsProtoBufMsgConf<SM>
    bool ProtoBufStream<S,M...>::send_req_quit(uint32_t curr_tag,const typename SM::PB_MSG_TYPE& m)
    {
        auto buf = ProtoBufMsg::write_msg<SM>(m);
        if(!buf)
            return false;
        return req_quit(curr_tag,{*buf});
    }
}

#undef MQAS_PB_STREAM_TEMPLATE_DECL
#endif //MQAS_CORE_PB_STREAM_IMPL_HPP

#pragma clang diagnostic pop