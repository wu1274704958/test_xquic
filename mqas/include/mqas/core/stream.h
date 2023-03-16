//
// Created by Administrator on 2023/3/1.
//

#pragma clang diagnostic push
#pragma ide diagnostic ignored "HidingNonVirtualFunction"
#ifndef MQAS_STREAM_H
#define MQAS_STREAM_H

#include <variant>
#include <mqas/core/connect.h>
#include <mqas/core/protobuf_msg.h>

namespace mqas::core {

    class MQAS_EXTERN IStreamVariant : public IStream {
    public:
        //interface
        StreamVariantErrcode on_change(const std::span<uint8_t> &params,
                                                   std::array<uint8_t, stream_variant_msg::EXTRA_PARAMS_MAX_SIZE> &ret_buf,
                                                   size_t &buf_len);
        void on_peer_change_ret(StreamVariantErrcode code, const std::span<uint8_t> &params);
        StreamVariantErrcode on_peer_quit(const std::span<uint8_t>&,std::array<uint8_t, stream_variant_msg::EXTRA_PARAMS_MAX_SIZE>&,
                          size_t&);
        void on_peer_quit_ret(StreamVariantErrcode,const std::span<uint8_t>&);
        bool req_quit(uint32_t curr_tag,const std::span<uint8_t> &d={});
        [[nodiscard]] bool isWaitPeerChangeRet() const;
        void setIsWaitPeerChangeRet(bool isWaitPeerChangeRet);
    protected:
        bool is_wait_peer_change_ret_:1 = false;
    };

    template<typename T>
    concept variability_stream_require = requires {
        requires std::is_default_constructible_v<T>;
        requires std::is_base_of_v<IStreamVariant,T>;
        requires HasStreamTag<T>;
    };
    template<typename ... S>
    requires requires{
        requires (variability_stream_require<S> && ...);
    }
    class StreamVariant : public IStream{
    public:
        size_t do_read();
        size_t do_read_shell();
        size_t do_read_hold();
        void do_write();
        void on_close();
        void on_reset(StreamAspect how);
        //operator functions
        bool write(const std::span<uint8_t>&);
        bool want_read(bool) const;
        bool want_write(bool) const;

        [[nodiscard]] void* get_cxt() const;
        void set_cxt(void*);
        void clear_read_buf();
        [[nodiscard]] bool has_unread_data() const;
        std::span<const uint8_t> read(size_t sz);
        std::span<const uint8_t> read_all();
        [[nodiscard]] std::span<const uint8_t> read_all_not_move() const;
        [[nodiscard]] size_t  unread_size() const;

        size_t on_read(const std::span<const uint8_t>& current);
        void on_peer_change_ret(StreamVariantErrcode code,const std::span<uint8_t>& params);
        StreamVariantErrcode change_to(size_t tag,const std::span<uint8_t>& change_params,
                                       std::array<uint8_t,stream_variant_msg::EXTRA_PARAMS_MAX_SIZE>& ret_buf,
                                       size_t& buf_len);

        void clear_curr_stream();
        template<typename CS>
        requires variability_stream_require<CS>
        bool req_change(const std::span<uint8_t>& change_params = {});
        template<typename CS,typename MP>
        requires variability_stream_require<CS> && IsProtoBufMsgConf<MP>
        bool req_change(const typename MP::PB_MSG_TYPE&);
        template<typename CS>
        requires variability_stream_require<CS>
        StreamVariantErrcode change_self(const std::span<uint8_t>& change_params,
                                         std::array<uint8_t,stream_variant_msg::EXTRA_PARAMS_MAX_SIZE>& ret_buf,
                                         size_t& buf_len);

        template<class CS>
        requires variability_stream_require<CS>
        CS* get_holds_stream();
        [[nodiscard]] bool has_holds_stream() const;
        StreamVariantErrcode on_peer_quit(const std::span<uint8_t> &,std::array<uint8_t, stream_variant_msg::EXTRA_PARAMS_MAX_SIZE>&,
        size_t&);
        void on_peer_quit_ret(StreamVariantErrcode,const std::span<uint8_t>&);
        bool req_quit(uint32_t curr_tag,const std::span<uint8_t> &d={});
        [[nodiscard]] bool isWaitPeerChangeRet() const;
        void setIsWaitPeerChangeRet(bool isWaitPeerChangeRet);
    protected:
        template<typename CS>
        requires (std::is_base_of_v<IStream,CS>)
        size_t do_read_curr(CS& cs);
        template<typename CS, typename F,typename ... Ss>
        requires requires{
            requires variability_stream_require<CS> && variability_stream_require<F>;
            requires (variability_stream_require<Ss> && ...);
        }
        StreamVariantErrcode change_self_inside(const std::span<uint8_t>& change_params,
                                                std::array<uint8_t,stream_variant_msg::EXTRA_PARAMS_MAX_SIZE>& ret_buf,
                                                size_t& buf_len);
        template<typename CS>
        requires variability_stream_require<CS>
        StreamVariantErrcode change_self_inside([[maybe_unused]] const std::span<uint8_t>& change_params);
        template<typename CS>
        requires variability_stream_require<CS>
        StreamVariantErrcode change_to_uncheck(const std::span<uint8_t>& change_params,
                                               std::array<uint8_t,stream_variant_msg::EXTRA_PARAMS_MAX_SIZE>& ret_buf,
                                               size_t& buf_len,bool is_req = false);
        template<typename CS, typename F,typename ... Ss>
        requires requires{
            requires variability_stream_require<CS> && variability_stream_require<F>;
            requires (variability_stream_require<Ss> && ...);
        }
        bool req_change_to([[maybe_unused]] const std::span<uint8_t>& change_params);
        template<typename CS>
        requires variability_stream_require<CS>
        bool req_change_to([[maybe_unused]] const std::span<uint8_t>& change_params);
    protected:
            std::variant<std::monostate,S...> stream_var_;
            size_t stream_tag_ = 0;
    };
}
#include "stream.impl.hpp"

#endif //MQAS_STREAM_H

#pragma clang diagnostic pop