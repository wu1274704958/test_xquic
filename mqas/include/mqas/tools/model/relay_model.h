#pragma once
#include <uv.h>
#include <memory>
#include <mqas/tools/stream/relay_stream.h>
#include <mqas/core/def.h>
#include <array>
#include <unordered_map>
#include <mqas/tools/unique_id_generator.h>
#include <mqas/io/ip.h>

namespace mqas::tools::relay{

struct peer_data{
    ::sockaddr addr;
    ::sockaddr connect_addr;
    std::weak_ptr<core::IStreamVariant> stream;
};

struct relay_pair{
    uint32_t id;
    std::array<peer_data,2> peers;
};

enum class RelayState : uint8_t {
    invaild = 0, 
    waiting = 1,
    relaying = 2,
    mismatching = 3,
};

class relay_model{
    public:
    std::pair<RelayState,uint32_t> try_connect(const ::sockaddr& addr,const ::sockaddr& connect_addr,std::weak_ptr<core::IStreamVariant> stream);
    std::weak_ptr<relay_pair> get_relay_pair(uint32_t id) const;
    bool remove_relay(uint32_t);
    bool remove_waiting(const ::sockaddr& addr,const ::sockaddr& connect_addr);
    template<typename T>
    std::shared_ptr<T> find_other_peer_stream(uint32_t id,const ::sockaddr& connect_addr) const
    {
        if(_relay_map.contains(id))
        {
            auto pair = _relay_map.at(id);
            for(auto& it : pair->peers)
            {
                if(io::Ip::compare_ip(it.addr,connect_addr))
                {
                    auto ptr = it.stream.lock();
                    if(ptr == nullptr)
                        return nullptr;
                    return std::dynamic_pointer_cast<T>(ptr);
                }
            }
        }
        return nullptr;
    }
    protected:
    std::unordered_multimap<::sockaddr,peer_data> _waiting_map;
    std::unordered_map<uint32_t,std::shared_ptr<relay_pair>> _relay_map;
    unique_id_generator<uint32_t> _id_generator;
};

}