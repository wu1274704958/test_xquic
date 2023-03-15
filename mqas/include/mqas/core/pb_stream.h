//
// Created by Administrator on 2023/3/10.
//

#pragma clang diagnostic push
#pragma ide diagnostic ignored "HidingNonVirtualFunction"
#ifndef MQAS_CORE_PB_STREAM_H
#define MQAS_CORE_PB_STREAM_H

#include <mqas/macro.h>
#include <mqas/core/stream.h>
#include <mqas/core/proto/msg_wrapper.pb.h>
#include <sigc++/sigc++.h>

namespace mqas::core{

    template<class S,class ... M>
    requires requires{ requires (IsProtoBufMsgConf<M> && ...);}
    class ProtoBufStream : public IStreamVariant {
    public:
        using HANDLER_FUN_TY = sigc::signal<void(size_t,const std::shared_ptr<google::protobuf::Message>&)>;
        using PARSER_FUN_TY = std::function<std::shared_ptr<google::protobuf::Message>(const proto::MsgWrapper&,MsgOrigin)>;
        ProtoBufStream();
        sigc::connection add_handler(size_t mid,const HANDLER_FUN_TY::slot_type& f);
        [[nodiscard]] bool has_handler(size_t mid)const;
        [[nodiscard]] size_t get_handlers_count(size_t mid)const;
        void erase_all_handlers(size_t mid);

        StreamVariantErrcode on_change_with_params(const std::span<uint8_t> &params,
                                                   std::array<uint8_t, stream_variant_msg::EXTRA_PARAMS_MAX_SIZE> &ret_buf,
                                                   size_t &buf_len);
        StreamVariantErrcode on_change_with_params(size_t,const std::shared_ptr<google::protobuf::Message>&,
                                                   std::array<uint8_t, stream_variant_msg::EXTRA_PARAMS_MAX_SIZE> &ret_buf,
                                                   size_t &buf_len);

        void on_peer_change_ret(StreamVariantErrcode code, const std::span<uint8_t> &params);
        void on_peer_change_ret(StreamVariantErrcode code, size_t,const std::shared_ptr<google::protobuf::Message>&);
        size_t on_read(const std::span<const uint8_t>& current);
        void on_read(size_t,const std::shared_ptr<google::protobuf::Message>&);
        StreamVariantErrcode on_peer_quit(const std::span<uint8_t>&,
                std::array<uint8_t,stream_variant_msg::EXTRA_PARAMS_MAX_SIZE>&,
                                          size_t&);
        void on_peer_quit_ret(StreamVariantErrcode,const std::span<uint8_t>&);
        StreamVariantErrcode on_peer_quit(size_t,const std::shared_ptr<google::protobuf::Message>&,
                                          std::array<uint8_t,stream_variant_msg::EXTRA_PARAMS_MAX_SIZE>&,
                                          size_t&);
        void on_peer_quit_ret(StreamVariantErrcode,size_t,const std::shared_ptr<google::protobuf::Message>&);

        template<class SM>
        requires IsProtoBufMsgConf<SM>
        bool write_msg_no_wrap(const std::span<uint8_t>& buf,size_t& sz,const typename SM::PB_MSG_TYPE&) const;
        template<class SM>
        requires IsProtoBufMsgConf<SM>
        std::optional<std::vector<uint8_t>> write_msg_no_wrap(const typename SM::PB_MSG_TYPE&) const;
        template<class SM>
        requires IsProtoBufMsgConf<SM>
        bool write_msg(const std::span<uint8_t>& buf,size_t& sz,const typename SM::PB_MSG_TYPE&) const;
        template<class SM>
        requires IsProtoBufMsgConf<SM>
        std::optional<std::vector<uint8_t>> write_msg(const typename SM::PB_MSG_TYPE&) const;
        template<class SM>
        requires IsProtoBufMsgConf<SM>
        bool send(const typename SM::PB_MSG_TYPE&);
        template<class SM>
        requires IsProtoBufMsgConf<SM>
        bool send_req_quit(uint32_t curr_tag,const typename SM::PB_MSG_TYPE&);
    protected:
        std::shared_ptr<proto::MsgWrapper> parse_base_msg(const std::span<uint8_t>&);
        void init_msg_parsers();
        template<class CM>
        void init_msg_parser();

        std::unordered_map<size_t ,PARSER_FUN_TY> msg_parsers_;
        std::unordered_map<size_t ,HANDLER_FUN_TY> msg_handlers_;
    };
}

#include "pb_stream.impl.hpp"

#endif //MQAS_CORE_PB_STREAM_H

#pragma clang diagnostic pop