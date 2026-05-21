#pragma once
#include <mqas/macro.h>
#include <mqas/core/pb_stream.h>
#include <mqas/tools/stream/MsgDef.h>
#include <sigc++/sigc++.h>

namespace mqas::tools::p2p_direct {

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
    std::string _name;
    std::string _peer_name;
    uint32_t _peer_id = 0;
    uint32_t _self_id = 0;
    bool _connected:1 = false;
};

} // namespace mqas::tools::p2p_direct

#include "p2p_direct_stream.hpp"

