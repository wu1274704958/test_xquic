#pragma once

#include <sigc++/sigc++.h>

namespace mqas::core{

template<typename T>
concept IsVaildSocket = requires(T t) {
    t.recv_start();
    requires std::is_same_v<sigc::signal<void(io::UdpSocket*, const std::optional<std::span<uint8_t>>&, ssize_t nread, const sockaddr*, unsigned)>,
    std::remove_cv_t<decltype(T::on_recv_signal)>>;
    new T();
    t.bind(std::declval<const sockaddr &>(),std::declval<uv_udp_flags>());
    t.get_sock_addr(std::declval<sockaddr&>());
    t.get_peer_addr(std::declval<sockaddr&>());
    std::is_same_v<std::remove_cv_t<decltype(t.try_send(std::declval<const std::vector<std::span<uint8_t>>&>(), std::declval<const sockaddr&>()))>, int>;
};

}