#include "mqas/core/engine_settings.h"
#include "mqas/core/engine_driver.h"
#include "mqas/io/ip.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>
#include <filesystem>
#include <functional>
#include <stdexcept>
#include <cctype>

namespace mqas::core {
namespace {
#define MQAS_LSQUIC_FIELDS(X) \
    X(es_versions) \
    X(es_cfcw) \
    X(es_sfcw) \
    X(es_max_cfcw) \
    X(es_max_sfcw) \
    X(es_max_streams_in) \
    X(es_handshake_to) \
    X(es_idle_conn_to) \
    X(es_silent_close) \
    X(es_max_header_list_size) \
    X(es_sttl) \
    X(es_pdmd) \
    X(es_aead) \
    X(es_kexs) \
    X(es_max_inchoate) \
    X(es_support_srej) \
    X(es_support_push) \
    X(es_support_tcid0) \
    X(es_support_nstp) \
    X(es_honor_prst) \
    X(es_send_prst) \
    X(es_progress_check) \
    X(es_rw_once) \
    X(es_proc_time_thresh) \
    X(es_pace_packets) \
    X(es_clock_granularity) \
    X(es_cc_algo) \
    X(es_cc_rtt_thresh) \
    X(es_noprogress_timeout) \
    X(es_init_max_data) \
    X(es_init_max_stream_data_bidi_remote) \
    X(es_init_max_stream_data_bidi_local) \
    X(es_init_max_stream_data_uni) \
    X(es_init_max_streams_bidi) \
    X(es_init_max_streams_uni) \
    X(es_idle_timeout) \
    X(es_ping_period) \
    X(es_scid_len) \
    X(es_scid_iss_rate) \
    X(es_qpack_dec_max_size) \
    X(es_qpack_dec_max_blocked) \
    X(es_qpack_enc_max_size) \
    X(es_qpack_enc_max_blocked) \
    X(es_ecn) \
    X(es_allow_migration) \
    X(es_retry_token_duration) \
    X(es_ql_bits) \
    X(es_spin) \
    X(es_delayed_acks) \
    X(es_timestamps) \
    X(es_max_udp_payload_size_rx) \
    X(es_grease_quic_bit) \
    X(es_dplpmtud) \
    X(es_base_plpmtu) \
    X(es_max_plpmtu) \
    X(es_mtu_probe_timer) \
    X(es_datagrams) \
    X(es_optimistic_nat) \
    X(es_ext_http_prio) \
    X(es_qpack_experiment) \
    X(es_ptpc_periodicity) \
    X(es_ptpc_max_packtol) \
    X(es_ptpc_dyn_target) \
    X(es_ptpc_target) \
    X(es_ptpc_prop_gain) \
    X(es_ptpc_int_gain) \
    X(es_ptpc_err_thresh) \
    X(es_ptpc_err_divisor) \
    X(es_delay_onclose) \
    X(es_max_batch_size) \
    X(es_check_tp_sanity) \
    X(es_amp_factor) \
    X(es_send_verneg)

template<class T> std::strong_ordering cmp(const T& a, const T& b) {
    if constexpr (std::is_floating_point_v<T>) {
        if (!std::isfinite(a) || !std::isfinite(b)) throw std::invalid_argument("non-finite LSQUIC setting");
    }
    return a < b ? std::strong_ordering::less : a > b ? std::strong_ordering::greater : std::strong_ordering::equal;
}
std::strong_ordering compare_addr(const sockaddr& a, const sockaddr& b) {
    if (auto c = cmp(a.sa_family, b.sa_family); c != 0) return c;
    if (a.sa_family == AF_INET) {
        const auto& x = reinterpret_cast<const sockaddr_in&>(a); const auto& y = reinterpret_cast<const sockaddr_in&>(b);
        if (auto c = cmp(ntohl(x.sin_addr.s_addr), ntohl(y.sin_addr.s_addr)); c != 0) return c;
        return cmp(ntohs(x.sin_port), ntohs(y.sin_port));
    }
    if (a.sa_family == AF_INET6) {
        const auto& x = reinterpret_cast<const sockaddr_in6&>(a); const auto& y = reinterpret_cast<const sockaddr_in6&>(b);
        for (size_t i = 0; i < 16; ++i) if (auto c = cmp(x.sin6_addr.s6_addr[i], y.sin6_addr.s6_addr[i]); c != 0) return c;
        if (auto c = cmp(x.sin6_scope_id, y.sin6_scope_id); c != 0) return c;
        return cmp(ntohs(x.sin6_port), ntohs(y.sin6_port));
    }
    throw std::invalid_argument("unsupported address family");
}
void combine(size_t& seed, size_t value) { seed ^= value + size_t{0x9e3779b9} + (seed << 6) + (seed >> 2); }
template<class T> void add(size_t& seed, T value) {
    if constexpr (std::is_floating_point_v<T>) {
        if (!std::isfinite(value)) throw std::invalid_argument("non-finite LSQUIC setting");
        if (value == 0) value = 0;
    }
    combine(seed, std::hash<T>{}(value));
}
const toml::value* find_field(const toml::value& table, std::string_view name) {
    for (const auto& [key, value] : table.as_table())
        if (key == name) return &value;
    return nullptr;
}
void add_addr(size_t& seed, const sockaddr& a) {
    add(seed, a.sa_family);
    if (a.sa_family == AF_INET) {
        const auto& x = reinterpret_cast<const sockaddr_in&>(a); add(seed, ntohl(x.sin_addr.s_addr)); add(seed, ntohs(x.sin_port)); return;
    }
    if (a.sa_family == AF_INET6) {
        const auto& x = reinterpret_cast<const sockaddr_in6&>(a);
        for (auto b : x.sin6_addr.s6_addr) add(seed, b);
        add(seed, x.sin6_scope_id); add(seed, ntohs(x.sin6_port)); return;
    }
    throw std::invalid_argument("unsupported address family");
}
}
std::strong_ordering EngineSettingsView::operator<=>(const EngineSettingsView& b) const {
    if (auto c = compare_addr(bind_addr, b.bind_addr); c != 0) return c;
#define COMPARE_FIELD(field) if (auto c = cmp(settings.field, b.settings.field); c != 0) return c;
    MQAS_LSQUIC_FIELDS(COMPARE_FIELD)
#if LSQUIC_WEBTRANSPORT_SERVER_SUPPORT
    COMPARE_FIELD(es_webtransport_server) COMPARE_FIELD(es_max_webtransport_server_streams)
#endif
#undef COMPARE_FIELD
    if (auto c = cmp(settings.es_ua != nullptr, b.settings.es_ua != nullptr); c != 0) return c;
    if (settings.es_ua && settings.es_ua != b.settings.es_ua) if (auto c = std::string_view(settings.es_ua) <=> std::string_view(b.settings.es_ua); c != 0) return c;
    for (size_t i = 0; i < sizeof(settings.es_preferred_address); ++i)
        if (auto c = cmp(settings.es_preferred_address[i], b.settings.es_preferred_address[i]); c != 0) return c;
    if (auto c = ssl_cert_path <=> b.ssl_cert_path; c != 0) return c;
    if (auto c = ssl_key_path <=> b.ssl_key_path; c != 0) return c;
    if (auto c = alpn <=> b.alpn; c != 0) return c;
    if (auto c = certificate_pin <=> b.certificate_pin; c != 0) return c;
    if (auto c = cmp(static_cast<uint32_t>(flags), static_cast<uint32_t>(b.flags)); c != 0) return c;
    if (tls_context == b.tls_context) return std::strong_ordering::equal;
    return std::less<const TlsConfiguration*>{}(tls_context, b.tls_context) ? std::strong_ordering::less : std::strong_ordering::greater;
}
bool EngineSettingsView::operator==(const EngineSettingsView& b) const { return (*this <=> b) == 0; }
EngineSettingsHash EngineSettingsHasher::operator()(const EngineSettingsView& v) const {
    size_t h = 0; add_addr(h, v.bind_addr);
#define HASH_FIELD(field) add(h, v.settings.field);
    MQAS_LSQUIC_FIELDS(HASH_FIELD)
#if LSQUIC_WEBTRANSPORT_SERVER_SUPPORT
    HASH_FIELD(es_webtransport_server) HASH_FIELD(es_max_webtransport_server_streams)
#endif
#undef HASH_FIELD
    add(h, v.settings.es_ua != nullptr);
    if (v.settings.es_ua) add(h, std::string_view(v.settings.es_ua));
    for (auto b : v.settings.es_preferred_address) add(h, b);
    add(h, v.ssl_cert_path); add(h, v.ssl_key_path); add(h, v.alpn); add(h, v.certificate_pin);
    add(h, static_cast<uint32_t>(v.flags)); add(h, v.tls_context);
    return {h};
}
EngineSettingsView make_engine_settings_view(const sockaddr& addr, const engine_config& config,
    const toml::value& origin, EngineFlags flags, const TlsConfiguration* tls) {
    std::string_view pin;
    const auto& table = origin.at("engine_config");
    if ((static_cast<uint32_t>(flags) & static_cast<uint32_t>(EngineFlags::Server)) == 0)
        if (const auto* value = find_field(table, "server_cert_sha256")) pin = value->as_string();
    return {addr, config.lsquic_settings, config.ssl_cert_path, config.ssl_key_path, config.alpn, pin, flags, tls};
}
const ::lsquic_engine_settings& default_lsquic_engine_settings(EngineFlags flags) {
    static const auto defaults = [] {
        std::array<::lsquic_engine_settings, 4> values{};
        for (unsigned i = 0; i < values.size(); ++i) ::lsquic_engine_init_settings(&values[i], i);
        return values;
    }();
    const auto index = static_cast<unsigned>(flags);
    if (index >= defaults.size()) throw std::invalid_argument("unsupported engine flags");
    return defaults[index];
}
EngineSettingsView prepare_engine_settings(const toml::value& config, EngineFlags flags,
    const sockaddr& addr, ::lsquic_engine_settings& storage, const TlsConfiguration* tls) {
    const auto& table = config.at("engine_config");
    const auto string_field = [&table](std::string_view name) -> std::string_view {
        if (const auto* value = find_field(table, name)) return value->as_string();
        return {};
    };
    const ::lsquic_engine_settings* settings = &default_lsquic_engine_settings(flags);
    if (config.contains("lsquic_settings")) {
        engine_driver::init_settings(storage, config, flags);
        settings = &storage;
    }
    auto pin = string_field("server_cert_sha256");
    if (!pin.empty() && (pin.size() != 64 || !std::all_of(pin.begin(), pin.end(), [](unsigned char c) { return std::isxdigit(c) != 0; })))
        throw std::invalid_argument("invalid certificate pin");
    if ((static_cast<uint32_t>(flags) & static_cast<uint32_t>(EngineFlags::Server)) != 0) pin = {};
    if (tls && !pin.empty()) throw std::invalid_argument("external TLS context cannot use certificate pin");
    return {addr, *settings, string_field("ssl_cert_path"), string_field("ssl_key_path"), string_field("alpn"), pin, flags, tls};
}
bool bind_endpoints_overlap(const sockaddr& a, const sockaddr& b) noexcept {
    if (a.sa_family != b.sa_family) return false;
    if (a.sa_family == AF_INET6) {
        const auto& x = reinterpret_cast<const sockaddr_in6&>(a);
        const auto& y = reinterpret_cast<const sockaddr_in6&>(b);
        return x.sin6_port != 0 && x.sin6_port == y.sin6_port &&
            (IN6_IS_ADDR_UNSPECIFIED(&x.sin6_addr) || IN6_IS_ADDR_UNSPECIFIED(&y.sin6_addr) ||
                io::Ip::compare_ip(a, b));
    }
    if (a.sa_family != AF_INET) return false;
    const auto& x = reinterpret_cast<const sockaddr_in&>(a); const auto& y = reinterpret_cast<const sockaddr_in&>(b);
    return x.sin_port != 0 && x.sin_port == y.sin_port && (x.sin_addr.s_addr == y.sin_addr.s_addr || x.sin_addr.s_addr == INADDR_ANY || y.sin_addr.s_addr == INADDR_ANY);
}
#undef MQAS_LSQUIC_FIELDS
}
