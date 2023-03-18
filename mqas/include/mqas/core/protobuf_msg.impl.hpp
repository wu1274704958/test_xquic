//
// Created by Administrator on 2023/3/16.
//

#include <mqas/core/proto/msg_wrapper.pb.h>
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

        template<class SM>
        requires IsProtoBufMsgConf<SM>
        bool ProtoBufMsg::write_msg(const std::span<uint8_t>& buf,size_t& sz,const typename SM::PB_MSG_TYPE& m)
        {
            auto bytes = write_msg_no_wrap<SM>(m);
            if(!bytes) return false;
            proto::MsgWrapper wrapper;
            wrapper.set_msg_id(SM::PB_MSG_ID);
            wrapper.set_body_len((uint32_t)bytes->size());
            wrapper.set_msg_body((const char*)bytes->data(),bytes->size());
            if((sz = wrapper.ByteSizeLong()) > buf.size())
                return false;
            if(!wrapper.SerializePartialToArray(buf.data(),buf.size()))
                return false;
            return true;
        }

        template<class SM>
        requires IsProtoBufMsgConf<SM>
        std::optional<std::vector<uint8_t>> ProtoBufMsg::write_msg(const typename SM::PB_MSG_TYPE&m)
        {
            auto bytes = write_msg_no_wrap<SM>(m);
            if(!bytes) return {};
            proto::MsgWrapper wrapper;
            wrapper.set_msg_id(SM::PB_MSG_ID);
            wrapper.set_body_len((uint32_t)bytes->size());
            wrapper.set_msg_body((const char*)bytes->data(),bytes->size());
            std::vector<uint8_t> buf(wrapper.ByteSizeLong());
            if(!wrapper.SerializePartialToArray(buf.data(),buf.size()))
                return {};
            return buf;
        }
    } // mqas
} // core