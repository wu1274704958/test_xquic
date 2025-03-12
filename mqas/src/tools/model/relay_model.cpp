#include <mqas/tools/model/relay_model.h>
#include <mqas/io/ip.h>

namespace mqas::tools::relay{

std::pair<RelayState,uint32_t> relay_model::try_connect(const ::sockaddr& addr,const boost::uuids::uuid& token,std::weak_ptr<core::IStreamVariant> stream)
{
    if(_waiting_map.contains(token))
    {
        auto range = _waiting_map.equal_range(token);
        for (auto it = range.first; it != range.second; ++it) {
            auto id = _id_generator.next();
            std::array<peer_data,2> arr;
            arr[0] = it->second;
            arr[1] = peer_data{ .addr = addr,.token = token, .stream = stream };
            _relay_map.insert({ id , std::make_shared<relay_pair>(relay_pair{ .id = id, .peers = arr }) });
            _waiting_map.erase(it);
            return {RelayState::relaying , id };
        }
    }else{
        _waiting_map.insert({ token , peer_data{ .addr = addr,.token = token, .stream = stream }});
        return { RelayState::waiting , 0 };
    }
    return { RelayState::waiting , 0 };
}

std::weak_ptr<relay_pair> relay_model::get_relay_pair(uint32_t id) const
{
    if(_relay_map.contains(id))
        return _relay_map.at(id);
    return {};
}

bool relay_model::remove_relay(uint32_t id)
{
    if(_relay_map.contains(id))
    {
        _relay_map.erase(id);
        _id_generator.remove(id);
        return true;
    }
    return false;
}

bool relay_model::remove_waiting(const ::sockaddr& addr,const boost::uuids::uuid& token)
{
    if(_waiting_map.contains(token))
    {
        auto _ = _waiting_map.equal_range(token);
        return true;
    }
    return false;
}
    
}