#pragma once
#include <boost/uuid/uuid.hpp>
#include <string>
#include <optional>

namespace mqas::comm::uuid
{
    std::optional<boost::uuids::uuid> to_uuid(const std::string& data);
    uint64_t to_high_64(const boost::uuids::uuid& uuid);
    uint64_t to_low_64(const boost::uuids::uuid& uuid);
} // namespace mqas::comm::uuid
