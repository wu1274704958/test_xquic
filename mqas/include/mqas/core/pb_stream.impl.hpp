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

    template<typename T,typename I,typename M, typename = std::void_t<>>
    struct has_member_function_on_read : std::false_type {};

    template<typename T,typename I,typename M>
    struct has_member_function_on_read<T,I,M,std::void_t<decltype(std::declval<T>().on_read(std::declval<I>(),std::declval<std::shared_ptr<M>>()))>>
            : std::true_type {};
    MQAS_PB_STREAM_TEMPLATE_DECL
    ProtoBufStream<S,M...>::ProtoBufStream()
    {
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
        auto [it,ok] = msg_parsers_.try_emplace(CM::PB_MSG_ID,[this](const proto::MsgWrapper& wrapper,MsgOrigin origin)->std::shared_ptr<google::protobuf::Message>{
            auto msg = std::make_shared<MSG_T>();
            if(!msg->ParseFromString(wrapper.msg_body()))
            {
                LOG(ERROR) << "Try parse proto buf message " << CM::PB_MSG_ID << " failed";
                return nullptr;
            }
            auto base_msg = std::dynamic_pointer_cast<google::protobuf::Message>(msg);
            //forward msg by origin
            if(origin == MsgOrigin::normal) {
                if constexpr (has_member_function_on_read<S,size_t,MSG_T>::value)
                {
                     static_cast<S*>(this)->on_read(CM::PB_MSG_ID,msg);
                }
                if(msg_handlers_.contains(CM::PB_MSG_ID))
                    msg_handlers_[CM::PB_MSG_ID].emit(CM::PB_MSG_ID,base_msg);
            }
            return base_msg;
        });
        if(!ok)
            LOG(WARNING) << "Not insert proto buf message handler " << CM::PB_MSG_ID << " cause by already has one";
    }
    MQAS_PB_STREAM_TEMPLATE_DECL
    std::shared_ptr<proto::MsgWrapper> ProtoBufStream<S,M...>::parse_base_msg(const std::span<uint8_t>& d)
    {
        auto msg = std::make_shared<proto::MsgWrapper>();
        if(!msg->ParseFromArray(d.data(),d.size()))
        {
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
    StreamVariantErrcode ProtoBufStream<S,M...>::on_change_with_params(const std::span<uint8_t> &params,
                                               std::array<uint8_t, stream_variant_msg::EXTRA_PARAMS_MAX_SIZE> &ret_buf,
                                               size_t &buf_len)
    {
        auto msg_wrap = parse_base_msg(params);
        if(!msg_wrap) {
            LOG(ERROR) << "Try parse proto::MsgWrapper failed on_change_with_params";
            return StreamVariantErrcode::parse_failed;
        }
        size_t mid = msg_wrap->msg_id();
        if(!msg_parsers_.contains(mid)){
            LOG(ERROR) << "Not support msg "<< mid <<" on_change_with_params";
            return StreamVariantErrcode::not_support;
        }
        auto msg = msg_parsers_[mid](*msg_wrap,MsgOrigin::on_change_with_params);
        return static_cast<S*>(this)->on_change_with_params(mid,msg,ret_buf,buf_len);
    }
    MQAS_PB_STREAM_TEMPLATE_DECL
    StreamVariantErrcode ProtoBufStream<S,M...>::on_change_with_params(size_t,const std::shared_ptr<google::protobuf::Message>&,
                                               std::array<uint8_t, stream_variant_msg::EXTRA_PARAMS_MAX_SIZE> &ret_buf,
                                               size_t &buf_len)
    {
        return StreamVariantErrcode::ok;
    }
    MQAS_PB_STREAM_TEMPLATE_DECL
    void ProtoBufStream<S,M...>::on_peer_change_ret(StreamVariantErrcode code, const std::span<uint8_t> &params)
    {
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
        static_cast<S*>(this)->on_peer_change_ret(code,mid,msg);
    }
    MQAS_PB_STREAM_TEMPLATE_DECL
    void ProtoBufStream<S,M...>::on_peer_change_ret(StreamVariantErrcode code, size_t,const std::shared_ptr<google::protobuf::Message>&){}

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
        static_cast<S*>(this)->on_read(mid,msg);
        return msg_wrap->ByteSizeLong();
    }
    MQAS_PB_STREAM_TEMPLATE_DECL
    void ProtoBufStream<S,M...>::on_read(size_t,const std::shared_ptr<google::protobuf::Message>&){}
    MQAS_PB_STREAM_TEMPLATE_DECL
    void ProtoBufStream<S,M...>::on_peer_quit_ret(StreamVariantErrcode e,const std::span<uint8_t>& d)
    {
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
        return static_cast<S*>(this)->on_peer_quit_ret(e,mid,msg);
    }
    MQAS_PB_STREAM_TEMPLATE_DECL
    void ProtoBufStream<S,M...>::on_peer_quit_ret(StreamVariantErrcode,size_t,const std::shared_ptr<google::protobuf::Message>&){}

    MQAS_PB_STREAM_TEMPLATE_DECL
    StreamVariantErrcode ProtoBufStream<S,M...>::on_peer_quit(const std::span<uint8_t> & d,
                                              std::array<uint8_t,stream_variant_msg::EXTRA_PARAMS_MAX_SIZE>& ret_buf,
                                              size_t& buf_len)
    {
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
        return static_cast<S*>(this)->on_peer_quit(mid,msg,ret_buf,buf_len);
    }
    MQAS_PB_STREAM_TEMPLATE_DECL
    StreamVariantErrcode ProtoBufStream<S,M...>::on_peer_quit(size_t,const std::shared_ptr<google::protobuf::Message>&,
                                              std::array<uint8_t,stream_variant_msg::EXTRA_PARAMS_MAX_SIZE>&,
                                              size_t&){
        return StreamVariantErrcode::ok;
    }

    MQAS_PB_STREAM_TEMPLATE_DECL
    template<class SM>
    requires IsProtoBufMsgConf<SM>
    bool ProtoBufStream<S,M...>::write_msg_no_wrap(const std::span<uint8_t>& buf,size_t& sz,const typename SM::PB_MSG_TYPE& m) const
    {
        if((sz = m.ByteSizeLong()) > buf.size())
            return false;
        if(!m.SerializePartialToArray(buf.data(),buf.size()))
            return false;
        return true;
    }
    MQAS_PB_STREAM_TEMPLATE_DECL
    template<class SM>
    requires IsProtoBufMsgConf<SM>
    std::optional<std::vector<uint8_t>> ProtoBufStream<S,M...>::write_msg_no_wrap(const typename SM::PB_MSG_TYPE& m) const
    {
        std::vector<uint8_t> buf(m.ByteSizeLong());
        if(!m.SerializePartialToArray(buf.data(),buf.size()))
            return {};
        return buf;
    }
    MQAS_PB_STREAM_TEMPLATE_DECL
    template<class SM>
    requires IsProtoBufMsgConf<SM>
    bool ProtoBufStream<S,M...>::write_msg(const std::span<uint8_t>& buf,size_t& sz,const typename SM::PB_MSG_TYPE& m) const
    {
        auto bytes = write_msg_no_wrap<SM>(m);
        if(!bytes) return false;
        proto::MsgWrapper wrapper;
        wrapper.set_msg_id(SM::PB_MSG_ID);
        wrapper.set_msg_body(std::move(*bytes));
        if((sz = wrapper.ByteSizeLong()) > buf.size())
            return false;
        if(!wrapper.SerializePartialToArray(buf.data(),buf.size()))
            return false;
        return true;
    }
    MQAS_PB_STREAM_TEMPLATE_DECL
    template<class SM>
    requires IsProtoBufMsgConf<SM>
    std::optional<std::vector<uint8_t>> ProtoBufStream<S,M...>::write_msg(const typename SM::PB_MSG_TYPE&m) const
    {
        auto bytes = write_msg_no_wrap<SM>(m);
        if(!bytes) return {};
        proto::MsgWrapper wrapper;
        wrapper.set_msg_id(SM::PB_MSG_ID);
        wrapper.set_msg_body(std::move(*bytes));
        std::vector<uint8_t> buf(m.ByteSizeLong());
        if(!wrapper.SerializePartialToArray(buf.data(),buf.size()))
            return {};
        return buf;
    }
    MQAS_PB_STREAM_TEMPLATE_DECL
    template<class SM>
    requires IsProtoBufMsgConf<SM>
    bool ProtoBufStream<S,M...>::send(const typename SM::PB_MSG_TYPE& m)
    {
        auto buf = write_msg<SM>(m);
        if(!buf)
            return false;
        return write({*buf});
    }
    MQAS_PB_STREAM_TEMPLATE_DECL
    template<class SM>
    requires IsProtoBufMsgConf<SM>
    bool ProtoBufStream<S,M...>::send_req_quit(uint32_t curr_tag,const typename SM::PB_MSG_TYPE& m)
    {
        auto buf = write_msg<SM>(m);
        if(!buf)
            return false;
        return req_quit(curr_tag,{*buf});
    }
}

#undef MQAS_PB_STREAM_TEMPLATE_DECL
#endif //MQAS_CORE_PB_STREAM_IMPL_HPP

#pragma clang diagnostic pop