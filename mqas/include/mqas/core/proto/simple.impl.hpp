#include <mqas/comm/binary.hpp>
#include <cstring>

#define SIMPLE_PKG_TEMPLATE_DECL \
    template<typename LT>       \
    requires std::is_unsigned_v<LT>

namespace mqas::core::proto {

    SIMPLE_PKG_TEMPLATE_DECL
    uint8_t simple_pkg<LT>::calculate_checksum()
    {
        checksum = 0;
        std::array<uint8_t,sizeof (LT)> array {};
        comm::to_big_endian((LT)body.size(),array);
        for (uint8_t byte : array)
            checksum ^= byte;
        for (uint8_t byte : body)
            checksum ^= byte;
        return checksum;
    }
    SIMPLE_PKG_TEMPLATE_DECL
    std::tuple<std::optional<mqas::core::proto::simple_pkg<LT>>,size_t>  simple_pkg<LT>::parse_command(const std::span<const uint8_t> &buffer) {
        if(buffer.size() < SIZE_WITHOUT_PARAMS) return {{},0};
        LT param_len = comm::from_big_endian<LT>(std::span<uint8_t>((uint8_t*)&buffer[0],sizeof (LT)));
        size_t all_size = param_len + SIZE_WITHOUT_PARAMS;
        if(buffer.size() < all_size) return {{},0};
        const auto checksum = calculate_checksum(std::span<const uint8_t>(buffer.data(),all_size - 1));
        if(checksum != buffer[all_size - 1]) return {{},0};
        simple_pkg<LT> pkg;
        if(param_len > 0)
            pkg.body = std::span(const_cast<uint8_t*>(&buffer[sizeof(LT)]),param_len);
        pkg.checksum = checksum;
        return {pkg,all_size};
    }
    SIMPLE_PKG_TEMPLATE_DECL
    uint8_t simple_pkg<LT>::calculate_checksum(const std::span<const uint8_t> &d) {
        uint8_t checksum = 0;
        for (uint8_t byte : d) {
            checksum ^= byte;
        }
        return checksum;
    }

    SIMPLE_PKG_TEMPLATE_DECL
    std::optional<std::vector<uint8_t>> simple_pkg<LT>::generate() {
        if(body.size() > PARAMS_MAX_SIZE)
            return {};
        std::vector<uint8_t> buffer;
        std::array<uint8_t,sizeof (LT)> array {};
        comm::to_big_endian(static_cast<LT>(body.size()),array);
        buffer.insert(buffer.end(),array.begin(),array.end());
        buffer.insert(buffer.end(), body.begin(), body.end());
        calculate_checksum();
        buffer.push_back(checksum);
        return buffer;
    }

}

#undef SIMPLE_PKG_TEMPLATE_DECL