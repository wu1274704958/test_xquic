#include <mqas/comm/binary.hpp>
#include <cstring>
#include <zlib.h>
#include <assert.h>

#define SIMPLE_PKG_TEMPLATE_DECL                                \
    template<typename LT,typename CST>                          \
    requires std::is_unsigned_v<LT> && std::is_unsigned_v<CST>

namespace mqas::core::proto {

    SIMPLE_PKG_TEMPLATE_DECL
    std::tuple<std::optional<mqas::core::proto::simple_pkg<LT,CST>>,size_t>  simple_pkg<LT,CST>::parse_command(const std::span<const uint8_t> &buffer) {
        if(buffer.size() < SIZE_WITHOUT_PARAMS) return {{},0};
        LT param_len = comm::from_big_endian<LT>(std::span<uint8_t>((uint8_t*)&buffer[0],sizeof (LT)));
        size_t all_size = param_len + SIZE_WITHOUT_PARAMS;
        if(buffer.size() < all_size) return {{},0};
        const auto checksum = calculate_checksum(std::span<const uint8_t>(buffer.data(),all_size - sizeof(CST)));

        if(!eq_checksum(std::span(const_cast<uint8_t*>(&buffer[all_size - sizeof(CST)]), sizeof(CST)),checksum))
            return {{},0};
        simple_pkg<LT> pkg;
        if(param_len > 0)
            pkg.body = std::span(const_cast<uint8_t*>(&buffer[sizeof(LT)]),param_len);
        pkg.checksum = checksum;
        return {pkg,all_size};
    }

    SIMPLE_PKG_TEMPLATE_DECL
    CST simple_pkg<LT,CST>::calculate_checksum(const std::span<const uint8_t> &d) {
        uint32_t checksum = crc32(0, d.data(), d.size());
        return static_cast<CST>(checksum & CHECKSUM_MAX_SIZE);
    }

    SIMPLE_PKG_TEMPLATE_DECL
    std::optional<std::vector<uint8_t>> simple_pkg<LT,CST>::generate() {
        if(body.size() > PARAMS_MAX_SIZE)
            return {};
        std::vector<uint8_t> buffer;
        std::array<uint8_t,sizeof (LT)> array {};
        comm::to_big_endian(static_cast<LT>(body.size()),array);
        buffer.insert(buffer.end(),array.begin(),array.end());
        buffer.insert(buffer.end(), body.begin(), body.end());
        push_checksum(buffer,calculate_checksum({ buffer.begin(),buffer.end()}));
        return buffer;
    }
    SIMPLE_PKG_TEMPLATE_DECL
    void simple_pkg<LT, CST>::push_checksum(std::vector<uint8_t>& buf, CST v)
    {
        if constexpr (sizeof(CST) == 1)
        {
            buf.push_back(v);
        }
        else {
            std::array<uint8_t, sizeof(CST)> array{};
            comm::to_big_endian(v, array);
            buf.insert(buf.end(), array.begin(), array.end());
        }
    }
    SIMPLE_PKG_TEMPLATE_DECL
    bool simple_pkg<LT, CST>::eq_checksum(const std::span<uint8_t>& buf, CST v)
    {
        assert(sizeof(CST) == buf.size());
        if constexpr (sizeof(CST) == 1)
        {
            return v == buf[0];
        }
        else {
            CST cksum = comm::from_big_endian<CST>(buf);
            return cksum == v;
        }
    }

}

#undef SIMPLE_PKG_TEMPLATE_DECL