//
// Created by Administrator on 2023/3/10.
//

#pragma clang diagnostic push
#pragma ide diagnostic ignored "HidingNonVirtualFunction"
#ifndef MQAS_CORE_PB_STREAM_H
#define MQAS_CORE_PB_STREAM_H

#include <mqas/macro.h>
#include <mqas/core/stream.h>
#include <sigc++/sigc++.h>

namespace mqas::core{

    template<class S,class ... M>
    requires requires{ requires (IsProtoBufMsgConf<M> && ...);}
    class ProtoBufStream : public IStreamVariant {
    public:
        using HANDLER_FUN_TY = sigc::signal<void(size_t,const std::shared_ptr<google::protobuf::Message>&)>;
        using PARSER_FUN_TY = std::function<std::shared_ptr<google::protobuf::Message>(const MsgHeader&,MsgOrigin)>;
        void on_init(::lsquic_stream_t* lsquic_stream,connect_cxt* connect_cxt);
        sigc::connection add_handler(size_t mid,const HANDLER_FUN_TY::slot_type& f);
        [[nodiscard]] bool has_handler(size_t mid)const;
        [[nodiscard]] size_t get_handlers_count(size_t mid)const;
        void erase_all_handlers(size_t mid);

        StreamVariantErrcode on_change(const std::span<uint8_t> &params,
                                                   std::vector<uint8_t> &ret_buf);
        StreamVariantErrcode on_change_msg(size_t,const std::shared_ptr<google::protobuf::Message>&,
                                                   std::vector<uint8_t> &ret_buf);

        void on_peer_change_ret(StreamVariantErrcode code, const std::span<uint8_t> &params);
        void on_peer_change_ret_msg(StreamVariantErrcode code, size_t,const std::shared_ptr<google::protobuf::Message>&);
        size_t on_read(const std::span<const uint8_t>& current);
        void on_read_msg(size_t,const std::shared_ptr<google::protobuf::Message>&);
        StreamVariantErrcode on_peer_quit(const std::span<uint8_t>&,
                std::vector<uint8_t>&);
        void on_peer_quit_ret(StreamVariantErrcode,const std::span<uint8_t>&);
        StreamVariantErrcode on_peer_quit_msg(size_t,const std::shared_ptr<google::protobuf::Message>&,
                                          std::vector<uint8_t>&);
        void on_peer_quit_ret_msg(StreamVariantErrcode,size_t,const std::shared_ptr<google::protobuf::Message>&);
        template<class SM>
        requires IsProtoBufMsgConf<SM>
        bool send(const typename SM::PB_MSG_TYPE&);
        template<class SM>
        requires IsProtoBufMsgConf<SM>
        bool send_req_quit(uint32_t curr_tag,const typename SM::PB_MSG_TYPE&);
        template<class SM,stream_variant_cmd C>
        requires IsProtoBufMsgConf<SM>
        bool send_sv_msg(const typename SM::PB_MSG_TYPE&,uint32_t p1,uint16_t p2,uint8_t p3,StreamVariantErrcode errcode);
    protected:
        [[nodiscard]] std::optional<MsgHeader> parse_base_msg(const std::span<uint8_t>&) const;
        void init_msg_parsers();
        template<class CM>
        void init_msg_parser();

        template<class F,class ... Ss>
        StreamVariantErrcode on_change_msg_forward(size_t,const std::shared_ptr<google::protobuf::Message>&,
                std::vector<uint8_t> &ret_buf);
        template<class F,class ... Ss>
        void on_peer_change_ret_msg_forward(StreamVariantErrcode code, size_t,const std::shared_ptr<google::protobuf::Message>&);
        template<class F,class ... Ss>
        StreamVariantErrcode on_peer_quit_msg_forward(size_t,const std::shared_ptr<google::protobuf::Message>&,
                                              std::vector<uint8_t>&);
        template<class F,class ... Ss>
        void on_peer_quit_ret_msg_forward(StreamVariantErrcode,size_t,const std::shared_ptr<google::protobuf::Message>&);
        std::unordered_map<size_t ,PARSER_FUN_TY> msg_parsers_;
        std::unordered_map<size_t ,HANDLER_FUN_TY> msg_handlers_;
    };
}

#include "pb_stream.impl.hpp"

#endif //MQAS_CORE_PB_STREAM_H

#pragma clang diagnostic pop