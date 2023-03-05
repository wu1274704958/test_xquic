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

    template<typename T>
    concept HasStreamTag = requires {
        requires std::is_same_v<typename std::remove_cv<decltype(T::STREAM_TAG)>::type ,size_t>;
    };
    enum class stream_variant_cmd : uint8_t {
        req_use_stream_tag = 1,

    };
    enum class stream_variant_errcode : uint8_t {
        ok = 0,
        failed = 1,
    };
    struct MQAS_EXTERN stream_variant_msg{
        stream_variant_cmd cmd;
        uint32_t param1 = 0;
        uint16_t param2 = 0;
        uint8_t param3 = 0;
        stream_variant_errcode errcode = stream_variant_errcode::ok;
        [[nodiscard]] std::vector<uint8_t> generate() const;
        static std::tuple<std::optional<stream_variant_msg>,size_t> parse_command(const std::span<const uint8_t>& buffer);
    };
    template<typename T>
    concept variability_stream_require = requires {
        requires std::is_default_constructible_v<T>;
        requires std::is_base_of_v<IStream,T>;
        requires HasStreamTag<T>;
    };
    template<typename ... S>
    requires requires{
        requires (variability_stream_require<S> && ...);
    }
    class StreamVariant : public IStream{
    public:
        size_t do_read();
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
        bool change_to(size_t tag);
        void clear_curr_stream();

        template<typename CS>
        requires variability_stream_require<CS>
        bool req_change()
        {
            return req_change_to<CS,S...>();
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
        size_t do_read_curr(CS& cs) const
        {
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
        bool change_to()
        {
            if constexpr(std::is_same_v<F,CS>)
            {
                change_to_uncheck<CS>();
                return true;
            }else{
                return change_to<CS,Ss...>();
            }
        }
        template<typename CS>
        requires variability_stream_require<CS>
        bool change_to()
        {
            return false;
        }
        template<typename CS>
        requires variability_stream_require<CS>
        void change_to_uncheck()
        {
            clear_curr_stream();
            stream_tag_ = CS::STREAM_TAG;
            stream_var_ = CS{};
            auto& stream = std::get<CS>(stream_var_);
            stream.set_cxt(cxt_);
            stream.on_init(stream_,connect_cxt_);
        }
        template<typename CS, typename F,typename ... Ss>
        requires requires{
            requires variability_stream_require<CS> && variability_stream_require<F>;
            requires (variability_stream_require<Ss> && ...);
        }
        bool req_change_to()
        {
            if constexpr(std::is_same_v<F,CS>)
            {
                stream_variant_msg msg{};
                msg.cmd = stream_variant_cmd::req_use_stream_tag;
                msg.param1 = static_cast<uint32_t >(CS::STREAM_TAG);
                auto data = msg.generate();
                write({data});
                return true;
            }else{
                return change_to<CS,Ss...>();
            }
        }
        template<typename CS>
        requires variability_stream_require<CS>
        bool req_change_to()
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