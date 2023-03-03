//
// Created by Administrator on 2023/3/1.
//
#include <mqas/core/stream.h>
#include <mqas/core/proto/simple.h>
#include <mqas/comm/binary.hpp>

std::vector<uint8_t> mqas::core::stream_variant_msg::generate() const {
    proto::simple_pkg pkg;
    pkg.type = static_cast<uint8_t>(cmd);
    std::vector<uint8_t> params;
    auto p1 = comm::to_big_endian(param1);//4
    auto p2 = comm::to_big_endian(param2);//2
    auto p3 = comm::to_big_endian(param3);//1
    auto err = comm::to_big_endian(errcode);//1
    params.insert(params.end(),p1.begin(),p1.end());
    params.insert(params.end(),p2.begin(),p2.end());
    params.insert(params.end(),p3.begin(),p3.end());
    params.insert(params.end(),err.begin(),err.end());
    pkg.parameters = std::span<uint8_t >(params.begin(),params.end());
    return pkg.generate();
}

std::tuple<std::optional<mqas::core::stream_variant_msg>,size_t> mqas::core::stream_variant_msg::parse_command(const std::span<const uint8_t> &buffer) {
    auto [pkg,use_len] = proto::simple_pkg::parse_command(buffer);
    if(!pkg) return {{},0};
    if(pkg->parameters.size() != 8) return {{},0};
    stream_variant_msg msg{};
    msg.cmd = static_cast<stream_variant_cmd>(pkg->type);
    msg.param1 = comm::from_big_endian<uint32_t>(std::span(&pkg->parameters[0],sizeof(uint32_t)));
    msg.param2 = comm::from_big_endian<uint16_t>(std::span(&pkg->parameters[4],sizeof(uint16_t)));
    msg.param3 = pkg->parameters[6];
    msg.errcode = static_cast<stream_variant_errcode>(pkg->parameters[7]);
    return {msg,use_len};
}
