//
// Created by Administrator on 2023/3/1.
//

#pragma clang diagnostic push
#pragma ide diagnostic ignored "HidingNonVirtualFunction"
#ifndef MQAS_STREAM_H
#define MQAS_STREAM_H

#include <variant>
#include <stack>
#include <mqas/core/connect.h>
#include <mqas/core/protobuf_msg.h>
#include <sigc++/sigc++.h>

namespace mqas::core {


    class MQAS_EXTERN IStreamVariantMgr : public IStream {
    public:
        virtual void on_req_quit() = 0;
        //-1 error; 0 success; 1 success but not write;2 write but failed
        virtual int on_send_sv_msg(const stream_variant_msg& msg, std::vector<uint8_t>& buf) = 0;
        virtual size_t on_read(const std::span<const uint8_t>& current) = 0;
    };

    class MQAS_EXTERN IStreamVariant : public IStream {
    public:
        //interface

        StreamVariantErrcode on_change(const std::span<uint8_t> &params,
                                                   std::vector<uint8_t> &ret_buf);
        StreamVariantErrcode on_local_change(const std::span<uint8_t>& params,
                                                    std::vector<uint8_t>& ret_buf);
        void on_peer_change_ret(StreamVariantErrcode code, const std::span<uint8_t> &params);
        StreamVariantErrcode on_peer_quit(const std::span<uint8_t>&,std::vector<uint8_t>&);
        void on_peer_quit_ret(StreamVariantErrcode,const std::span<uint8_t>&);
        bool req_quit(uint32_t curr_tag,const std::span<uint8_t> &d={},bool lazy = false);
        [[nodiscard]] bool isWaitPeerChangeRet() const;
        void setIsWaitPeerChangeRet(bool isWaitPeerChangeRet);
        [[nodiscard]] size_t getStreamTag() const;
        void setStreamTag(size_t streamTag);
        virtual void on_req_quit();
        [[nodiscard]] std::shared_ptr<IStreamVariantMgr> get_outer() const;
        void set_outer(std::weak_ptr<IStreamVariantMgr> outer);
    protected:
        bool is_wait_peer_change_ret_:1 = false;
        size_t stream_tag_ = 0;
        std::weak_ptr<IStreamVariantMgr> outer;
    };

    template<typename T>
    concept variability_stream_pair_require = requires {
        requires std::is_default_constructible_v<typename T::STREAM_TYPE>;
        requires std::is_base_of_v<IStreamVariant,typename T::STREAM_TYPE>;
        requires HasStreamTag<T>;
    };
    template<typename T>
    concept variability_stream_require = requires {
        requires std::is_default_constructible_v<T>;
        requires std::is_base_of_v<IStreamVariant,T>;
    };

    enum class variant_stream_state : uint8_t
    {
        none = 0,
        req_wait_ack = 1,
        half_req = 2,
        active = 3,
        half_active = 4, // skip and after manual send ack
        quit_wait_ack = 5,
        half_quit = 6 // skip and after manual send ack
    };

    template<typename ... S>
    requires requires{
        requires (variability_stream_pair_require<S> && ...);
    }
    class StreamVariant : public IStreamVariantMgr,public std::enable_shared_from_this<StreamVariant<S...>> {
    public:
        sigc::signal<void(std::shared_ptr<IStreamVariant>)> on_change_stream_signal;
        sigc::signal<void(std::shared_ptr<IStreamVariant>)> on_quit_stream_signal;
        sigc::signal<void(std::shared_ptr<IStreamVariant>)> on_resume_stream_signal;
        sigc::signal<void(std::shared_ptr<IStreamVariant>)> on_pause_stream_signal;
        sigc::signal<void(std::shared_ptr<IStreamVariant>)> on_close_signal;
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

        size_t on_read(const std::span<const uint8_t>& current) override;
        void on_peer_change_ret(StreamVariantErrcode code,const std::span<uint8_t>& params);
        StreamVariantErrcode change_to(size_t tag,const std::span<uint8_t>& change_params,
                                       std::vector<uint8_t>& ret_buf);

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
                                         std::vector<uint8_t>& ret_buf);

        template<class CS>
        requires variability_stream_require<CS>
        std::shared_ptr<CS> get_holds_stream();
        std::shared_ptr<IStreamVariant> get_holds_stream(size_t stream_tag);
        [[nodiscard]] bool has_holds_stream() const;
        StreamVariantErrcode on_peer_quit(const std::span<uint8_t> &,std::vector<uint8_t>&);
        void on_peer_quit_ret(StreamVariantErrcode,const std::span<uint8_t>&);
        bool req_quit(uint32_t curr_tag,const std::span<uint8_t> &d={},bool lazy = false);
        [[nodiscard]] bool isWaitPeerChangeRet() const;
        void setIsWaitPeerChangeRet(bool isWaitPeerChangeRet);
    protected:
        template<typename CS>
        requires (std::is_base_of_v<IStream,CS>)
        size_t do_read_curr(CS& cs);
        template<typename CS, typename F,typename ... Ss>
        requires requires{
            requires variability_stream_require<CS> && variability_stream_pair_require<F>;
            requires (variability_stream_pair_require<Ss> && ...);
        }
        StreamVariantErrcode change_self_inside(const std::span<uint8_t>& change_params,
                                                std::vector<uint8_t>& ret_buf);
        template<typename CS>
        requires variability_stream_require<CS>
        StreamVariantErrcode change_self_inside([[maybe_unused]] const std::span<uint8_t>& change_params);
        template<typename CS,bool IS_LOCAL>
        requires variability_stream_pair_require<CS>
        StreamVariantErrcode change_to_uncheck(const std::span<uint8_t>& change_params,
                                               std::vector<uint8_t>& ret_buf);
        template<typename CS, typename F,typename ... Ss>
        requires requires{
            requires variability_stream_require<CS> && variability_stream_pair_require<F>;
            requires (variability_stream_pair_require<Ss> && ...);
        }
        bool req_change_to([[maybe_unused]] const std::span<uint8_t>& change_params);
        template<typename CS>
        requires variability_stream_require<CS>
        bool req_change_to([[maybe_unused]] const std::span<uint8_t>& change_params);
        template<typename CS>
        requires (std::is_base_of_v<IStream,CS>)
        void hold_stream_unread_moveto_shell(CS& cs);
        StreamVariantErrcode try_push_curr_stream();
        
        template<typename SP>
        requires variability_stream_pair_require<SP>
        void push_stream(std::shared_ptr<typename SP::STREAM_TYPE> stream);
        bool try_pop_stream();
        template<typename SP>
        requires variability_stream_pair_require<SP>
        void set_curr_stream(std::shared_ptr<IStreamVariant> stream);

        void on_req_quit() override;
        //-1 error; 0 success; 1 success but not write;2 write but failed
        int on_send_sv_msg(const stream_variant_msg& msg,std::vector<uint8_t>& buf) override;
    protected:
            std::variant<std::monostate,typename std::shared_ptr<typename S::STREAM_TYPE> ...> stream_var_;
            std::stack<std::pair<size_t,std::shared_ptr<IStreamVariant>>> stack;
            size_t stream_tag_ = 0;
            variant_stream_state current_state;
    };
}
#include "stream.impl.hpp"

#endif //MQAS_STREAM_H

#pragma clang diagnostic pop