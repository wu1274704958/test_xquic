//
// Created by Administrator on 2023/3/1.
//
#include <mqas/core/stream.h>
#include <mqas/comm/binary.hpp>

std::optional<std::vector<uint8_t>> mqas::core::stream_variant_msg::generate() const {
    if(extra_params.size() > EXTRA_PARAMS_MAX_SIZE) return {};
    proto::simple_pkg<uint32_t> pkg;
    std::vector<uint8_t> params;
    std::array<uint8_t,sizeof(param1)> p1{};
    comm::to_big_endian(param1,p1);
    std::array<uint8_t,sizeof(param2)> p2{};
    comm::to_big_endian(param2,p2);
    params.insert(params.end(),static_cast<uint8_t>(cmd));//1
    params.insert(params.end(),p1.begin(),p1.end());//4
    params.insert(params.end(),p2.begin(),p2.end());//2
    params.push_back(param3);                       //1
    params.push_back(static_cast<uint8_t>(errcode));//1
    if(!extra_params.empty()) {
        comm::to_big_endian((uint32_t)extra_params.size(),p1);
        params.insert(params.end(), p1.begin(), p1.end());//4
        params.insert(params.end(), extra_params.begin(), extra_params.end());//N
    }
    pkg.body_len = params.size();
    pkg.body = {params};
    return pkg.generate();
}

std::tuple<std::optional<mqas::core::stream_variant_msg>,size_t> mqas::core::stream_variant_msg::parse_command(const std::span<const uint8_t> &buffer) {
    auto [pkg,use_len] = proto::simple_pkg<uint32_t>::parse_command(buffer);
    if(!pkg) return {{},0};
    std::span<uint8_t>& parameters = pkg->body;
    stream_variant_msg msg{};
    auto size = parameters.size();
    constexpr auto min_size = 9 + sizeof(uint32_t);
    if(size < 9) return {{},0};
    else if(size > min_size){
        auto extra_size = comm::from_big_endian<uint32_t>(std::span(&parameters[9],sizeof(uint32_t))) ;
#if !NDEBUG
        assert(extra_size + min_size == parameters.size());
#endif
        if(extra_size + min_size != parameters.size())
            return {{},0};
        msg.extra_params = std::span<uint8_t >(&parameters[min_size],extra_size);
    }
    msg.cmd = static_cast<stream_variant_cmd>(parameters[0]);
    msg.param1 = comm::from_big_endian<uint32_t>(std::span(&parameters[1],sizeof(uint32_t)));
    msg.param2 = comm::from_big_endian<uint16_t>(std::span(&parameters[5],sizeof(uint16_t)));
    msg.param3 = parameters[7];
    msg.errcode = static_cast<StreamVariantErrcode>(parameters[8]);
    return {msg,use_len};
}

mqas::core::StreamVariantErrcode
mqas::core::IStreamVariant::on_change(const std::span<uint8_t>& params,
                                           std::vector<uint8_t>& ret_buf)
{
    return StreamVariantErrcode::ok;
}

void mqas::core::IStreamVariant::on_peer_change_ret(mqas::core::StreamVariantErrcode code, const std::span<uint8_t> &params) {

}

bool mqas::core::IStreamVariant::isWaitPeerChangeRet() const {
    return is_wait_peer_change_ret_;
}

void mqas::core::IStreamVariant::setIsWaitPeerChangeRet(bool isWaitPeerChangeRet) {
    is_wait_peer_change_ret_ = isWaitPeerChangeRet;
}

mqas::core::StreamVariantErrcode mqas::core::IStreamVariant::on_peer_quit(const std::span<uint8_t> &,std::vector<uint8_t>&) {
    return StreamVariantErrcode::ok;
}

bool mqas::core::IStreamVariant::req_quit(uint32_t curr_tag,const std::span<uint8_t> &d) {
    stream_variant_msg msg{};
    msg.cmd = stream_variant_cmd::req_quit_hold_stream;
    msg.param1 = curr_tag;
    msg.extra_params = d;
    auto data = msg.generate();
    if(!data)return false;
    bool ret = write({*data});
    if(ret) setIsWaitPeerChangeRet(true);
    return ret;
}

void mqas::core::IStreamVariant::on_peer_quit_ret(mqas::core::StreamVariantErrcode,const std::span<uint8_t> &) {}
