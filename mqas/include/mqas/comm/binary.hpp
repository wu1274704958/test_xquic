//
// Created by Administrator on 2023/3/2.
//

#ifndef MQAS_COMMBINARY_HPP
#define MQAS_COMMBINARY_HPP

#include <cstdint>
#include <vector>
#include <cstring>
namespace mqas::comm {
    template<typename T>
    std::vector<uint8_t> to_big_endian(T val) {
        std::vector<uint8_t> result(sizeof(T));
        if constexpr (std::endian::native == std::endian::big) {
            std::memcpy(result.data(), &val, sizeof(T));
        } else {
            unsigned char *data = reinterpret_cast<unsigned char *>(&val);
            for (int i = 0; i < sizeof(T); i++) {
                result[i] = data[sizeof(T) - i - 1];
            }
        }
        return result;
    }
    template<typename T>
    void to_big_endian(T val,std::array<uint8_t,sizeof(T)>& result) {
        if constexpr (std::endian::native == std::endian::big) {
            std::memcpy(result.data(), &val, sizeof(T));
        } else {
            unsigned char *data = reinterpret_cast<unsigned char *>(&val);
            for (int i = 0; i < sizeof(T); i++) {
                result[i] = data[sizeof(T) - i - 1];
            }
        }
    }

    template<typename T>
    T from_big_endian(const std::span<uint8_t> &data) {
        T result;
        if constexpr (std::endian::native == std::endian::big) {
            std::memcpy(&result, data.data(), sizeof(T));
        } else {
            uint8_t *temp_data = reinterpret_cast<uint8_t *>(&result);
            for (int i = 0; i < sizeof(T); i++) {
                temp_data[sizeof(T) - i - 1] = data[i];
            }
        }
        return result;
    }
}
#endif //MQAS_COMMBINARY_HPP
