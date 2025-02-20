#pragma clang diagnostic push
#pragma ide diagnostic ignored "HidingNonVirtualFunction"
#pragma once
#include <mqas/core/engine_base.h>
#include <mqas/macro.h>
#include <span>
#include <memory>
#include <vector>
#include <queue>

namespace mqas::core {

	struct engine_cxt;

	class MQAS_EXTERN IConnect {
	public:
		sigc::signal<void(const uint8_t*, size_t)> on_recv_datagram;
		void init(::lsquic_conn_t* conn, std::shared_ptr<engine_cxt> cxt);
		void on_close();
		void on_new_stream(::lsquic_stream_t* lsquic_stream);
		void on_stream_read(::lsquic_stream_t* lsquic_stream);
		void on_stream_write(::lsquic_stream_t* lsquic_stream);
		void on_stream_close(::lsquic_stream_t* lsquic_stream);

		//optional callback
		void on_goaway_received();
		ssize_t on_dg_write(void*, size_t);
		void on_datagram(const void* buf, size_t);
		void on_hsk_done(enum lsquic_hsk_status s);
		void on_new_token(const unsigned char* token, size_t token_size);
		void on_stream_reset(lsquic_stream_t* s, int how);
		void on_conncloseframe_received(int app_error, uint64_t error_code, const char* reason, int reason_len);
        //operator
        void close();
        void abort() const;
        void going_away() const;
        [[nodiscard]] const char*get_sni() const;
        bool get_sockaddr(const sockaddr** local,const sockaddr** peer) const;
        [[nodiscard]] size_t get_min_datagram_size() const;
        bool set_min_datagram_size(size_t sz) const;
        unsigned int cancel_pending_streams(unsigned int n) const;
        [[nodiscard]] unsigned int pending_streams() const;
        [[nodiscard]] unsigned int avail_streams() const;
        [[nodiscard]] LSQUIC_CONN_STATUS status(char* buf = nullptr,size_t buf_len = 0) const;
        void make_stream();

		//interface
		void set_cxt(void*);
		[[nodiscard]] void* get_cxt() const;
		[[nodiscard]] size_t datagram_buf_size() const;
		bool write_datagram(const std::span<uint8_t>&);
		[[nodiscard]] bool flush_datagram() const;
		bool write_stream(::lsquic_stream_t*,const std::span<uint8_t>&);
		[[nodiscard]] ::lsquic_hsk_status get_hsk_status() const;
        bool has_stream(lsquic_stream_t*) const;
	protected:
		::lsquic_conn_t* conn_ = nullptr;
		void* cxt_;
		std::shared_ptr<engine_cxt> engine_cxt_;
		std::vector<uint8_t> datagram_buf_;
		std::queue<short> datagram_queue_;
		size_t datagram_buf_write_p_ = 0;
		::lsquic_hsk_status hsk_status_ = ::lsquic_hsk_status::LSQ_HSK_FAIL;
		bool goaway_receive_:1 = false; 
	};

	template<typename C>
	requires requires{
		requires std::is_default_constructible_v<C>;
		requires std::is_base_of_v<IConnect, C>;
	}
	class engine : public mqas::core::IEngine,public std::enable_shared_from_this<engine<C>>
	{
	friend C;
	public:
		sigc::signal<void(std::shared_ptr<C>)> on_new_connect_signal;
		sigc::signal<void(std::shared_ptr<C>)> on_connect_closed_signal;
		std::weak_ptr<C> connect(const ::sockaddr& addr, ::lsquic_version ver, const char* hostname = nullptr, unsigned short base_plpmtu = 0,
			const unsigned char* sess_resume = nullptr, size_t sess_resume_len = 0,const unsigned char* token = nullptr, size_t token_sz = 0);

		void on_new_conn([[maybe_unused]] void* stream_if_ctx, lsquic_conn_t* lsquic_conn);
		void on_conn_closed(lsquic_conn_t* lsquic_conn);
		void on_new_stream([[maybe_unused]] void* stream_if_ctx, lsquic_stream_t* lsquic_stream);
		void on_read(lsquic_stream_t* lsquic_stream, [[maybe_unused]] lsquic_stream_ctx_t* lsquic_stream_ctx);
		void on_write(lsquic_stream_t* lsquic_stream, [[maybe_unused]] lsquic_stream_ctx_t* lsquic_stream_ctx);
		void on_close(lsquic_stream_t* lsquic_stream, [[maybe_unused]] lsquic_stream_ctx_t* lsquic_stream_ctx);
        void close();

		//optional callback
		void on_goaway_received(lsquic_conn_t* c);
		ssize_t on_dg_write(lsquic_conn_t* c, void*, size_t);
		void on_datagram(lsquic_conn_t*, const void* buf, size_t);
		void on_hsk_done(lsquic_conn_t* c, enum lsquic_hsk_status s);
		void on_new_token(lsquic_conn_t* c, const unsigned char* token, size_t token_size);
		void on_reset(lsquic_stream_t* s, [[maybe_unused]] lsquic_stream_ctx_t* h, int how);
		void on_conncloseframe_received(lsquic_conn_t* c, int app_error, uint64_t error_code, const char* reason, int reason_len);

		void init(std::shared_ptr<engine_cxt> cxt); //override
		bool contain(::lsquic_conn_t* conn) const;
		std::weak_ptr<C> add(::lsquic_conn_t* conn);
        bool write_datagram(::lsquic_conn_t* conn,const std::span<uint8_t>&);
        bool write_stream(::lsquic_conn_t* conn,lsquic_stream_t* stream,const std::span<uint8_t>&);
        bool has_stream(lsquic_conn_t* conn,lsquic_stream_t* stream) const;
		size_t connect_count() const;
	protected:
		std::unordered_map<size_t,std::shared_ptr<C>> conn_map_;
	};
}
#include "engine.impl.hpp"


#pragma clang diagnostic pop