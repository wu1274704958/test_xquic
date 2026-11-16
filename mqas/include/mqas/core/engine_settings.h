#pragma once
#include <mqas/macro.h>
#include <mqas/core/def.h>
#include <mqas/core/tls_configuration.h>
#include <compare>
#include <string_view>
#include <cstddef>

namespace mqas::core {
// A borrowed, immutable view; every referenced value must outlive the view.
struct EngineSettingsView {
    const sockaddr& bind_addr;
    const ::lsquic_engine_settings& settings;
    std::string_view ssl_cert_path, ssl_key_path, alpn, certificate_pin;
    EngineFlags flags;
    const TlsConfiguration* tls_context;
    MQAS_EXTERN std::strong_ordering operator<=>(const EngineSettingsView&) const;
    MQAS_EXTERN bool operator==(const EngineSettingsView&) const;
};
struct EngineSettingsHash {
    size_t value = 0;
    auto operator<=>(const EngineSettingsHash&) const = default;
};
struct MQAS_EXTERN EngineSettingsHasher {
    EngineSettingsHash operator()(const EngineSettingsView&) const;
};
// References configuration owned by the caller. The configuration must remain immutable.
MQAS_EXTERN EngineSettingsView make_engine_settings_view(const sockaddr& addr, const engine_config& config,
    const toml::value& origin, EngineFlags flags, const TlsConfiguration* tls = nullptr);
// Defaults have static storage duration and are safe for concurrent readers.
MQAS_EXTERN const ::lsquic_engine_settings& default_lsquic_engine_settings(EngineFlags flags);
// The returned view borrows addr, config, settings_storage (when used), and tls.
// All must remain alive and unchanged while the view is used. Normalize config
// paths with engine_driver::normalize_engine_config before comparing engines.
MQAS_EXTERN EngineSettingsView prepare_engine_settings(const toml::value& config, EngineFlags flags,
    const sockaddr& addr, ::lsquic_engine_settings& settings_storage, const TlsConfiguration* tls = nullptr);
// Wildcard overlap is not equality and must not be used as an unordered-map comparator.
MQAS_EXTERN bool bind_endpoints_overlap(const sockaddr& a, const sockaddr& b) noexcept;
}
