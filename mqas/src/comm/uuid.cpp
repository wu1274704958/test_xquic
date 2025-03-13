#include <mqas/comm/uuid.h>

namespace mqas::comm::uuid
{
    std::optional<boost::uuids::uuid> to_uuid(const std::string& data)
    {
        if(data.size() != sizeof(boost::uuids::uuid))
            return {};
        boost::uuids::uuid uuid;
        std::memcpy(&uuid,data.data(),sizeof(boost::uuids::uuid));
        return uuid;
    }
    uint64_t to_high_64(const boost::uuids::uuid& uuid)
    {
        uint64_t res = 0;
        std::memcpy(&res,&uuid,sizeof(uint64_t));
        return res;
    }
    uint64_t to_low_64(const boost::uuids::uuid& uuid)
    {
        uint64_t res = 0;
        std::memcpy(&res,((uint8_t*)&uuid) + sizeof(uint64_t),sizeof(uint64_t));
        return res;
    }
} // namespace mqas::comm::uuid
