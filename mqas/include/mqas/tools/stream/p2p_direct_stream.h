#pragma once
#include <mqas/macro.h>
#include <mqas/core/pb_stream.h>
#include <mqas/tools/stream/MsgDef.h>
#include <sigc++/sigc++.h>
#include <mutex>
#include <unordered_map>
#include <functional>

namespace mqas::tools::p2p_direct {

using DirectStreamPolicy = std::function<bool(void*, uint32_t, uint32_t&, std::string&)>;

class DirectStreamRegistry {
public:
    struct Entry {
        void* stream;
        uint32_t local = 0;
        uint32_t peer = 0;
        bool active = false;
    };

    std::shared_ptr<Entry> reserve_local(void* stream, uint32_t id) {
        return reserve(stream, id, true);
    }
    std::shared_ptr<Entry> reserve_peer(void* stream, uint32_t id) {
        return reserve(stream, id, false);
    }
    bool complete(const std::shared_ptr<Entry>& entry, uint32_t local, uint32_t peer) {
        std::lock_guard lock(mutex_);
        if (!entry || !local || !peer) return false;
        prune();
        const auto current = entries_.find(entry->stream);
        if (current == entries_.end() || current->second.lock() != entry) return false;
        for (const auto& [key, weak] : entries_) {
            const auto other = weak.lock();
            if (other && other != entry && (other->local == local || other->peer == peer)) return false;
        }
        entry->local = local;
        entry->peer = peer;
        entry->active = true;
        return true;
    }
    bool find_peer(uint32_t peer, void*& stream) {
        std::lock_guard lock(mutex_);
        prune();
        for (const auto& [key, weak] : entries_) {
            const auto entry = weak.lock();
            if (entry && entry->active && entry->peer == peer) {
                stream = key;
                return true;
            }
        }
        return false;
    }
    bool find_stream(void* stream, uint32_t& local, uint32_t& peer) {
        std::lock_guard lock(mutex_);
        prune();
        const auto it = entries_.find(stream);
        if (it == entries_.end()) return false;
        const auto entry = it->second.lock();
        if (!entry || !entry->active) return false;
        local = entry->local;
        peer = entry->peer;
        return true;
    }
private:
    void prune() {
        for (auto it = entries_.begin(); it != entries_.end();) {
            if (it->second.expired()) it = entries_.erase(it);
            else ++it;
        }
    }
    std::shared_ptr<Entry> reserve(void* stream, uint32_t id, bool local) {
        std::lock_guard lock(mutex_);
        if (!stream || !id) return {};
        prune();
        if (entries_.contains(stream)) return {};
        for (const auto& [key, weak] : entries_) {
            const auto entry = weak.lock();
            if (entry && (local ? entry->local : entry->peer) == id) return {};
        }
        auto entry = std::make_shared<Entry>();
        entry->stream = stream;
        if (local) entry->local = id;
        else entry->peer = id;
        entries_.emplace(stream, entry);
        return entry;
    }
    std::mutex mutex_;
    std::unordered_map<void*, std::weak_ptr<Entry>> entries_;
};

/**
 * P2PDirectStream — 直连协议 Stream，两端使用同一个类。
 * 作为 client 的一端主动调用 req_connect() 发起直连请求，
 * 作为 server 的一端在 on_change_msg_s 中处理请求并返回应答。
 * 任意一端均可调用 req_quit() 发起断开。
 */
template<typename S,typename ... MSG>
class P2PDirectStream : public core::ProtoBufStream<S,
    ReqDirectConnectPair,    RespondDirectConnectPair,
    ReqDirectQuitPair,       RespondDirectQuitPair, MSG...>
{
public:
    ~P2PDirectStream() { release_stream_id(); }
    void on_close() {
        release_stream_id();
        _connected = false;
        core::IStream::on_close();
    }
    sigc::signal<void(const std::string&)> on_rejected_signal;
    uint32_t local_stream_id() const { return _local_stream_id; }
    uint32_t peer_stream_id() const { return _peer_stream_id; }
    // ── signals ──────────────────────────────────────────────────────────
    /** server 端：对端发来连接请求，返回是否接受。 */
    sigc::signal<bool(const std::shared_ptr<proto::p2p_client::RequestDirectConnect>&)> on_connect_request_signal;

    sigc::signal<void(const std::string&,uint32_t)> on_connected_signal;

    sigc::signal<void(const std::string&,uint32_t)> on_disconnected_signal;

    sigc::signal<void(proto::p2p_client::RetCode,uint32_t)> on_connect_failed_signal;

    /**
     * 发起断开请求（双端均可调用）。
     * @param reason  可选的断开原因
     */
    bool req_quit(const std::string& reason = {},bool lazy = false);

    mqas::core::StreamVariantErrcode on_local_change_msg_s(const std::shared_ptr<proto::p2p_client::RequestDirectConnect>& msg,std::vector<uint8_t>& ret_buf);
    const std::string& get_name();

    // server 端：收到对端的 RequestDirectConnect
    core::StreamVariantErrcode on_change_msg_s(
        const std::shared_ptr<proto::p2p_client::RequestDirectConnect>& req,
        std::vector<uint8_t>& ret_buf);

    // client 端：收到 RequestDirectConnect 的应答
    void on_peer_change_ack(core::StreamVariantErrcode code, const std::span<uint8_t>& params) {
        if (!_awaiting_direct_ack) return;
        core::ProtoBufStream<S, ReqDirectConnectPair, RespondDirectConnectPair,
            ReqDirectQuitPair, RespondDirectQuitPair, MSG...>::on_peer_change_ack(code, params);
        if (_awaiting_direct_ack) {
            _awaiting_direct_ack = false;
            release_stream_id();
            on_rejected_signal.emit("invalid direct stream response");
            on_connect_failed(proto::p2p_client::unknown_error);
            if (code == core::StreamVariantErrcode::ok) this->close();
        }
    }
    void on_peer_change_ack_msg_s(core::StreamVariantErrcode code,
        const std::shared_ptr<proto::p2p_client::RespondDirectConnect>& msg);

    //收到对端的 RequestDirectQuit
    core::StreamVariantErrcode on_peer_quit_msg_s(const std::shared_ptr<proto::p2p_client::RequestDirectQuit>& req,
                                                            std::vector<uint8_t>& buf);

    void on_peer_quit_ack_msg_s(core::StreamVariantErrcode e,
        const std::shared_ptr<proto::p2p_client::RespondDirectQuit>& ack);

protected:
    virtual void on_connected(const std::string&);
    virtual void on_disconnected(const std::string&);
    virtual void on_connect_failed(proto::p2p_client::RetCode);

protected:
    void release_stream_id() {
        _entry.reset();
    }
    std::shared_ptr<DirectStreamRegistry::Entry> _entry;
    uint32_t _local_stream_id = 0;
    uint32_t _peer_stream_id = 0;
    std::string _name;
    std::string _peer_name;
    uint32_t _peer_id = 0;
    uint32_t _self_id = 0;
    bool _connected:1 = false;
    bool _awaiting_direct_ack = false;
};

} // namespace mqas::tools::p2p_direct

#include "p2p_direct_stream.hpp"

