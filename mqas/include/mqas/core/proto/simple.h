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
#pragma once

#include <limits>

#ifdef max
#undef max
#endif

namespace mqas::core::proto {
    // 命令格式定义
    template<typename LT,typename CST = uint16_t>
    requires std::is_unsigned_v<LT> && std::is_unsigned_v<CST>
    struct simple_pkg {
        using LEN_TY = LT;
        static constexpr size_t LEN_TY_SZ = sizeof(LT);
        static constexpr size_t CK_TY_SZ = sizeof(CST); // checksum
        std::span<uint8_t> body;
        CST checksum;

        std::optional<std::vector<uint8_t>> generate();
        static constexpr size_t SIZE_WITHOUT_PARAMS = sizeof(CST) + sizeof(LT);
        static constexpr size_t PARAMS_MAX_SIZE = std::numeric_limits<LT>::max();
        static constexpr size_t CHECKSUM_MAX_SIZE = std::numeric_limits<CST>::max();
        static std::tuple<std::optional<simple_pkg>,size_t> parse_command(const std::span<const uint8_t>& buffer);
        static CST calculate_checksum(const std::span<const uint8_t>&);

        static void push_checksum(std::vector<uint8_t>& buf,CST v);
        static bool eq_checksum(const std::span<uint8_t>& buf, CST v);
    };
}

#include "simple.impl.hpp"
#endif //MQAS_PROTO_SIMPLE_H
