//
// Created by wws on 2023/2/28.
//
#ifndef TEST_QUIC_CONNECT_H
#define TEST_QUIC_CONNECT_H

#include <mqas/macro.h>
#include <mqas/core/engine.h>

namespace mqas::core{


    class MQAS_EXTERN IStream {
    public:
        //interface
        void on_init(::lsquic_stream_t* lsquic_stream);
        void on_read();
        void on_write();
        void on_close();
        void on_reset(StreamAspect how);
        //operator functions
        bool write(const std::span<char>&);
        bool want_read() const;
        bool want_write() const;
        /**
        * 0: Stop reading.
        * 1: Stop writing.
        * 2: Stop both reading and writing.
        */
        bool shutdown(StreamAspect how);
        bool close() const;
        void clear_read_buf();
        bool on_read(const std::span<char>& current);
        bool has_unread_data() const;
        std::span<char> read(size_t sz);
        std::span<char> read_all();
        size_t  unread_size() const;
    protected:
        std::span<char> read_uncheck(size_t sz);
        void move_read_pos_uncheck(size_t sz);
        static size_t reader_read(void *lsqr_ctx, void *buf, size_t count);
        static size_t reader_size(void *lsqr_ctx);
        static size_t read_func(void *ctx, const unsigned char *buf, size_t len, int fin);

    protected:
        std::vector<char> buf_;
        std::vector<char> read_buf_;
        size_t buf_write_pos = 0;
        size_t buf_read_pos = 0;
        ::lsquic_stream_t* stream_ = nullptr;
        StreamAspect reset_val_ = StreamAspect::None;
        ::lsquic_reader reader_ = {};
        bool is_closed_:1 = false;
        bool want_read_on_init_:1 = true;
    };
}


#endif //TEST_QUIC_CONNECT_H
