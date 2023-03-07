//
// Created by wws on 2023/3/2.
//
#include <mqas/core/proto/simple.h>
#include <cstring>
#include <limits>

uint8_t mqas::core::proto::simple_pkg::calculate_checksum()
{
    checksum = type + static_cast<uint8_t >(parameters.size());
    for (uint8_t byte : parameters) {
        checksum += byte;
    }
    return checksum;
}

std::tuple<std::optional<mqas::core::proto::simple_pkg>,size_t>  mqas::core::proto::simple_pkg::parse_command(const std::span<const uint8_t> &buffer) {
    if(buffer.size() < SIZE_WITHOUT_PARAMS) return {{},0};
    uint8_t param_len = buffer[1];
    size_t all_size = param_len + SIZE_WITHOUT_PARAMS;
    if(buffer.size() < all_size) return {{},0};
    const auto checksum = calculate_checksum(std::span<const uint8_t>(buffer.data(),all_size - 1));
    if(checksum != buffer[all_size - 1]) return {{},0};
    simple_pkg pkg;
    pkg.type = buffer[0];
    if(param_len > 0)
        pkg.parameters = std::span(const_cast<uint8_t*>(&buffer[2]),param_len);
    pkg.checksum = checksum;
    return {pkg,all_size};
}

uint8_t mqas::core::proto::simple_pkg::calculate_checksum(const std::span<const uint8_t> &d) {
    uint8_t checksum = 0;
    for (uint8_t byte : d) {
        checksum += byte;
    }
    return checksum;
}

std::optional<std::vector<uint8_t>> mqas::core::proto::simple_pkg::generate() {
    if(parameters.size() > std::numeric_limits<uint8_t>::max())
        return {};
    std::vector<uint8_t> buffer;
    buffer.push_back(static_cast<uint8_t>(type));
    buffer.push_back(static_cast<uint8_t>(parameters.size()));
    buffer.insert(buffer.end(), parameters.begin(), parameters.end());
    calculate_checksum();
    buffer.push_back(checksum);
    return buffer;
}
