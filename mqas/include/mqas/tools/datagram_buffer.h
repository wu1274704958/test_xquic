#pragma once

#include "mqas/macro.h"
#include <queue>
#include <vector>
#include <span>
#include <optional>
#include <cstdint>

namespace mqas::tools{

    struct MQAS_EXTERN datagram_buffer
    {
    public:
        void push(const std::span<uint8_t>& buf);
        std::optional<std::span<uint8_t>> pop();
        size_t count() const;
        bool empty() const;

    private:
        std::queue<size_t> _split_queue;
        std::vector<uint8_t> _buffer;
        size_t _write_pos = 0;
        size_t _read_pos = 0;
    };
    
}