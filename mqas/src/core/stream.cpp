//
// Created by Administrator on 2023/3/1.
//
#include <mqas/core/stream.h>
#include <mqas/comm/binary.hpp>

std::optional<std::vector<uint8_t>> mqas::core::stream_variant_msg::generate() const {
    if(extra_params.size() > EXTRA_PARAMS_MAX_SIZE) return {};
    proto::simple_pkg pkg;
    pkg.type = static_cast<uint8_t>(cmd);
    std::vector<uint8_t> params;
    auto p1 = comm::to_big_endian(param1);
    auto p2 = comm::to_big_endian(param2);
    params.insert(params.end(),p1.begin(),p1.end());//4
    params.insert(params.end(),p2.begin(),p2.end());//2
    params.push_back(param3);                       //1
    params.push_back(static_cast<uint8_t>(errcode));//1
    if(!extra_params.empty()) {
        params.push_back(static_cast<uint8_t>(extra_params.size()));//1 or 0
        params.insert(params.end(), extra_params.begin(), extra_params.end());//N
    }
    pkg.parameters = std::span<uint8_t >(params.begin(),params.end());
    return pkg.generate();
}

std::tuple<std::optional<mqas::core::stream_variant_msg>,size_t> mqas::core::stream_variant_msg::parse_command(const std::span<const uint8_t> &buffer) {
    auto [pkg,use_len] = proto::simple_pkg::parse_command(buffer);
    if(!pkg) return {{},0};
    stream_variant_msg msg{};
    auto size = pkg->parameters.size();
    if(size < 8) return {{},0};
    else if(size > 8){
        auto extra_size = pkg->parameters[8];
#if DEBUG
        assert(extra_size + 8 + 1 == pkg->parameters.size());
#endif
        if(extra_size + 8 + 1 != pkg->parameters.size())
            return {{},0};
        msg.extra_params = std::span<uint8_t >(&pkg->parameters[9],extra_size);
    }
    msg.cmd = static_cast<stream_variant_cmd>(pkg->type);
    msg.param1 = comm::from_big_endian<uint32_t>(std::span(&pkg->parameters[0],sizeof(uint32_t)));
    msg.param2 = comm::from_big_endian<uint16_t>(std::span(&pkg->parameters[4],sizeof(uint16_t)));
    msg.param3 = pkg->parameters[6];
    msg.errcode = static_cast<StreamVariantErrcode>(pkg->parameters[7]);
    return {msg,use_len};
}
