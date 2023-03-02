//
// Created by wws on 2023/2/28.
//
#pragma clang diagnostic push
#pragma ide diagnostic ignored "HidingNonVirtualFunction"
#ifndef MQAS_CONNECT_H
#define MQAS_CONNECT_H

#include <mqas/macro.h>
#include <mqas/core/engine.h>

namespace mqas::core{

    struct connect_cxt;
    class MQAS_EXTERN IStream {
    public:
        //interface
        void on_init(::lsquic_stream_t* lsquic_stream,connect_cxt* connect_cxt);
        void do_read();
        void do_write();
        void on_close();
        void on_reset(StreamAspect how);
        //operator functions
        bool write(const std::span<uint8_t>&);
        bool want_read() const;
        bool want_write() const;
        /**
        * 0: Stop reading.
        * 1: Stop writing.
        * 2: Stop both reading and writing.
        */
        [[nodiscard]] void* get_cxt() const;
        void set_cxt(void*);
        bool shutdown(StreamAspect how);
        bool close() const;
        void clear_read_buf();
        bool on_read(const std::span<uint8_t>& current);
        [[nodiscard]] bool has_unread_data() const;
        std::span<const uint8_t> read(size_t sz);
        std::span<const uint8_t> read_all();
        [[nodiscard]] std::span<const uint8_t> read_all_not_move() const;
        [[nodiscard]] size_t  unread_size() const;
    protected:
        std::span<const uint8_t> read_uncheck(size_t sz);
        void move_read_pos_uncheck(size_t sz);
        static size_t reader_read(void *lsqr_ctx, void *buf, size_t count);
        static size_t reader_size(void *lsqr_ctx);
        static size_t read_func(void *ctx, const unsigned char *buf, size_t len, [[maybe_unused]] int fin);

    protected:
        std::vector<uint8_t> buf_;
        std::vector<uint8_t> read_buf_;
        size_t buf_write_pos = 0;
        size_t buf_read_pos = 0;
        ::lsquic_stream_t* stream_ = nullptr;
        StreamAspect reset_val_ = StreamAspect::None;
        ::lsquic_reader reader_ = {};
        connect_cxt* connect_cxt_ = nullptr;
        void* cxt_ = nullptr;
        bool is_closed_:1 = false;
        bool want_read_on_init_:1 = true;
    };
    struct connect_cxt{
        engine_cxt* engine_cxt_;
        std::function<bool(lsquic_stream_t*)> has_stream;
        std::function<bool(lsquic_stream_t*,const std::span<uint8_t>&)> write_stream;
    };
    template<typename S>
    requires requires{
        requires std::default_initializable<S>;
        requires std::is_base_of_v<IStream, S>;
    }
    class Connect : public IConnect{
    public:
        void init(::lsquic_conn_t* conn, engine_cxt* engine_cxt);
        void on_close();
        void on_new_stream(::lsquic_stream_t* lsquic_stream);
        void on_stream_read(::lsquic_stream_t* lsquic_stream);
        void on_stream_write(::lsquic_stream_t* lsquic_stream);
        void on_stream_close(::lsquic_stream_t* lsquic_stream);
        void on_stream_reset(lsquic_stream_t* s, int how);
        bool has_stream(lsquic_stream_t*) const;
        bool write_stream(::lsquic_stream_t*,const std::span<uint8_t>&);
    protected:
        std::unordered_map<size_t ,std::shared_ptr<S>> stream_map_;
        connect_cxt connect_cxt_;
    };

}
#include "connect.impl.hpp"

#endif //MQAS_CONNECT_H

#pragma clang diagnostic pop