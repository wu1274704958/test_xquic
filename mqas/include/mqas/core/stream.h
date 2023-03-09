//
// Created by Administrator on 2023/3/1.
//

#pragma clang diagnostic push
#pragma ide diagnostic ignored "HidingNonVirtualFunction"
#ifndef MQAS_STREAM_H
#define MQAS_STREAM_H

#include <variant>
#include <mqas/core/connect.h>

namespace mqas::core {

    class MQAS_EXTERN IStreamVariant : public IStream {
    public:
        //interface
        StreamVariantErrcode on_change(std::array<uint8_t, stream_variant_msg::EXTRA_PARAMS_MAX_SIZE> &ret_buf,
                                       size_t &buf_len);
        StreamVariantErrcode on_change_with_params(const std::span<uint8_t> &params,
                                                   std::array<uint8_t, stream_variant_msg::EXTRA_PARAMS_MAX_SIZE> &ret_buf,
                                                   size_t &buf_len);
        void on_peer_change_ret(StreamVariantErrcode code, const std::span<uint8_t> &params);
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
        bool req_change(const std::span<uint8_t>& change_params = {})
        {
            return req_change_to<CS,S...>(change_params);
        }
        template<typename CS>
        requires variability_stream_require<CS>
        StreamVariantErrcode change_self(const std::span<uint8_t>& change_params,
                                         std::array<uint8_t,stream_variant_msg::EXTRA_PARAMS_MAX_SIZE>& ret_buf,
                                         size_t& buf_len)
        {
            return change_self_inside<CS,S...>(change_params,ret_buf,buf_len);
        }

        template<class CS>
        requires variability_stream_require<CS>
        CS* get_holds_stream()
        {
            if(std::holds_alternative<CS>(stream_var_))
            {
                return &(std::get<CS>(stream_var_));
            }
            return nullptr;
        }

    protected:
        template<typename CS>
        requires (std::is_base_of_v<IStream,CS>)
        size_t do_read_curr(CS& cs)
        {
            if(cs.isWaitPeerChangeRet())
                return do_read_shell();
            const auto ret = cs.do_read();
            if(ret > 0) {
                const auto span = cs.read_all_not_move();
                if (const size_t read_len = cs.on_read(span);read_len > 0)
                    cs.move_read_pos_uncheck(read_len);
            }
            return ret;
        }
        template<typename CS, typename F,typename ... Ss>
        requires requires{
            requires variability_stream_require<CS> && variability_stream_require<F>;
            requires (variability_stream_require<Ss> && ...);
        }
        StreamVariantErrcode change_self_inside(const std::span<uint8_t>& change_params,
                                                std::array<uint8_t,stream_variant_msg::EXTRA_PARAMS_MAX_SIZE>& ret_buf,
                                                size_t& buf_len)
        {
            if constexpr(std::is_same_v<F,CS>)
            {
                return change_to_uncheck<CS>(change_params,ret_buf,buf_len);
            }else{
                return change_self_inside<CS,Ss...>(change_params,ret_buf,buf_len);
            }
        }
        template<typename CS>
        requires variability_stream_require<CS>
        StreamVariantErrcode change_self_inside(const std::span<uint8_t>& change_params)
        {
            return StreamVariantErrcode::failed_not_find;
        }
        template<typename CS>
        requires variability_stream_require<CS>
        StreamVariantErrcode change_to_uncheck(const std::span<uint8_t>& change_params,
                                               std::array<uint8_t,stream_variant_msg::EXTRA_PARAMS_MAX_SIZE>& ret_buf,
                                               size_t& buf_len,bool is_req = false)
        {
            clear_curr_stream();
            stream_tag_ = CS::STREAM_TAG;
            stream_var_ = CS{};
            auto& stream = std::get<CS>(stream_var_);
            StreamVariantErrcode res = change_params.empty() ? stream.on_change(ret_buf,buf_len) :
                    stream.on_change_with_params(change_params,ret_buf,buf_len);
            if(res != StreamVariantErrcode::ok) {
                clear_curr_stream();
                return res;
            }
            if(is_req)stream.setIsWaitPeerChangeRet(true);
            stream.set_cxt(cxt_);
            stream.on_init(stream_,connect_cxt_);
            return StreamVariantErrcode::ok;
        }
        template<typename CS, typename F,typename ... Ss>
        requires requires{
            requires variability_stream_require<CS> && variability_stream_require<F>;
            requires (variability_stream_require<Ss> && ...);
        }
        bool req_change_to(const std::span<uint8_t>& change_params)
        {
            if(change_params.size() > stream_variant_msg::EXTRA_PARAMS_MAX_SIZE)
                return false;
            if constexpr(std::is_same_v<F,CS>)
            {
                std::array<uint8_t,stream_variant_msg::EXTRA_PARAMS_MAX_SIZE> ret_buf{};
                std::size_t buf_len = 0;
                auto ret = change_to_uncheck<CS>(change_params,ret_buf,buf_len,true);
                if(ret != StreamVariantErrcode::ok)
                {
                    LOG(ERROR) << "req_change_to " << CS::STREAM_TAG << " change self failed error = " << (size_t)ret;
                    return false;
                }
                stream_variant_msg msg{};
                msg.cmd = stream_variant_cmd::req_use_stream_tag;
                msg.param1 = static_cast<uint32_t >(CS::STREAM_TAG);
                if(buf_len > 0)
                    msg.extra_params = std::span<uint8_t>(&ret_buf[0],buf_len);
                auto data = msg.generate();
                write({*data});
                return true;
            }else{
                return req_change_to<CS,Ss...>(change_params);
            }
        }
        template<typename CS>
        requires variability_stream_require<CS>
        bool req_change_to(const std::span<uint8_t>& change_params)
        {
            return false;
        }
    protected:
            std::variant<std::monostate,S...> stream_var_;
            size_t stream_tag_ = 0;
    };
}
#include "stream.impl.hpp"

#endif //MQAS_STREAM_H

#pragma clang diagnostic pop