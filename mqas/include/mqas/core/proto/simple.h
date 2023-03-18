//
// Created by wws on 2023/3/2.
//

#ifndef MQAS_PROTO_SIMPLE_H
#define MQAS_PROTO_SIMPLE_H
#include <mqas/macro.h>
#include <cstdint>
#include <vector>
#include <optional>
#include <span>
#include <tuple>
#include <limits>

#ifdef max
#undef max
#endif

namespace mqas::core::proto {
    // 命令格式定义
    template<typename LT>
    requires std::is_unsigned_v<LT>
    struct simple_pkg {
        LT body_len;
        std::span<uint8_t> body;
        uint8_t checksum;

        uint8_t calculate_checksum();
        std::optional<std::vector<uint8_t>> generate();
        static constexpr size_t SIZE_WITHOUT_PARAMS = sizeof(uint8_t) + sizeof(LT);
        static constexpr size_t PARAMS_MAX_SIZE = std::numeric_limits<LT>::max();
        static std::tuple<std::optional<simple_pkg>,size_t> parse_command(const std::span<const uint8_t>& buffer);
        static uint8_t calculate_checksum(const std::span<const uint8_t>&);
    };
}

#include "simple.impl.hpp"
#endif //MQAS_PROTO_SIMPLE_H
