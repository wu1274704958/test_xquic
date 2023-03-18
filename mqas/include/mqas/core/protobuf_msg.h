//
// Created by Administrator on 2023/3/16.
//

#ifndef MQAS_CORE_PROTOBUF_MSG_HPP
#define MQAS_CORE_PROTOBUF_MSG_HPP
#include <type_traits>
#include <google/protobuf/message.h>
#include <span>
#include <optional>
namespace mqas {
    namespace core {
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
        struct ProtoBufMsg{
            ///protobuf msg tool
            template<class SM>
            requires IsProtoBufMsgConf<SM>
            static bool write_msg_no_wrap(const std::span<uint8_t>& buf,size_t& sz,const typename SM::PB_MSG_TYPE&);
            template<class SM>
            requires IsProtoBufMsgConf<SM>
            static std::optional<std::vector<uint8_t>> write_msg_no_wrap(const typename SM::PB_MSG_TYPE&);
//            template<class SM>
//            requires IsProtoBufMsgConf<SM>
//            static bool write_msg(const std::span<uint8_t>& buf,size_t& sz,const typename SM::PB_MSG_TYPE&);
            template<class SM>
            requires IsProtoBufMsgConf<SM>
            static bool write_msg(std::vector<uint8_t>& buf,const typename SM::PB_MSG_TYPE&);
            template<class SM>
            requires IsProtoBufMsgConf<SM>
            static std::optional<std::vector<uint8_t>> write_msg(const typename SM::PB_MSG_TYPE&);
        };

    } // mqas
} // core

#include "protobuf_msg.impl.hpp"

#endif //MQAS_CORE_PROTOBUF_MSG_HPP
