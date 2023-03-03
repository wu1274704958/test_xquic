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

namespace mqas::core::proto {
    // 命令格式定义
    struct MQAS_EXTERN simple_pkg {
        uint8_t type;
        std::span<uint8_t> parameters;
        uint8_t checksum;

        uint8_t calculate_checksum();
        std::vector<uint8_t> generate();
        static constexpr size_t SIZE_WITHOUT_PARAMS = 3;
        static std::tuple<std::optional<simple_pkg>,size_t> parse_command(const std::span<const uint8_t>& buffer);
        static uint8_t calculate_checksum(const std::span<const uint8_t>&);
    };

    // 校验和计算函数

}
#endif //MQAS_PROTO_SIMPLE_H
