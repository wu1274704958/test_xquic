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

    template<typename T>
    concept IsProtoBufMsgConf = requires {
        requires std::is_same_v<typename std::remove_cv<decltype(T::PB_MSG_ID)>::type ,size_t>;
        requires std::is_base_of_v<google::protobuf::Message,typename T::PB_MSG_TYPE>;
    };

    template<size_t ID,typename T>
    struct PBMsgPair{
        constexpr static size_t PB_MSG_ID = ID;
        using PB_MSG_TYPE = T;
    };

    enum class MsgOrigin : uint8_t {
        normal = 0, // or_read
        on_change_with_params,
        on_peer_change_ret
    };

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
        void erase_all(size_t mid);

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