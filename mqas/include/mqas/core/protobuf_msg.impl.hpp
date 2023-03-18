//
// Created by Administrator on 2023/3/16.
//

#include <google/protobuf/message.h>
#include <mqas/core/def.h>
namespace mqas {
    namespace core {
        /// protobuf message gen
        template<class SM>
        requires IsProtoBufMsgConf<SM>
        bool ProtoBufMsg::write_msg_no_wrap(const std::span<uint8_t>& buf,size_t& sz,const typename SM::PB_MSG_TYPE& m)
        {
            if((sz = m.ByteSizeLong()) > buf.size())
                return false;
            if(!m.SerializePartialToArray(buf.data(),buf.size()))
                return false;
            return true;
        }
        template<class SM>
        requires IsProtoBufMsgConf<SM>
        std::optional<std::vector<uint8_t>> ProtoBufMsg::write_msg_no_wrap(const typename SM::PB_MSG_TYPE& m)
        {
            std::vector<uint8_t> buf(m.ByteSizeLong());
            if(!m.SerializePartialToArray(buf.data(),buf.size()))
                return {};
            return buf;
        }

//        template<class SM>
//        requires IsProtoBufMsgConf<SM>
//        bool ProtoBufMsg::write_msg(const std::span<uint8_t>& buf,size_t& sz,const typename SM::PB_MSG_TYPE& m)
//        {
//            auto bytes = write_msg_no_wrap<SM>(m);
//            if(!bytes) return false;
//            MsgHeader pkg;
//            pkg.msg_id = SM::PB_MSG_ID;
//            pkg.msg_body = bytes;
//            if((sz = pkg.byte_size()) > buf.size())
//                return false;
//            auto ret = pkg.generate();
//            if(!ret)
//                return false;
//            memcpy(buf.data(),ret->data(), ret->size());
//            return true;
//        }
        template<class SM>
        requires IsProtoBufMsgConf<SM>
        bool ProtoBufMsg::write_msg(std::vector<uint8_t>& buf,const typename SM::PB_MSG_TYPE& m)
        {
            auto bytes = write_msg_no_wrap<SM>(m);
            if(!bytes) return {};
            MsgHeader pkg;
            pkg.msg_id = SM::PB_MSG_ID;
            pkg.msg_body = {*bytes};
            auto ret = pkg.generate();
            if(!ret)
                return false;
            buf = std::move(*ret);
            return true;
        }

        template<class SM>
        requires IsProtoBufMsgConf<SM>
        std::optional<std::vector<uint8_t>> ProtoBufMsg::write_msg(const typename SM::PB_MSG_TYPE&m)
        {
            auto bytes = write_msg_no_wrap<SM>(m);
            if(!bytes) return {};
            MsgHeader pkg;
            pkg.msg_id = SM::PB_MSG_ID;
            pkg.msg_body = {*bytes};
            return pkg.generate();
        }
    } // mqas
} // core