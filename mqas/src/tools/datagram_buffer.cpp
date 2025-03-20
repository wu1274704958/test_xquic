#include <mqas/tools/datagram_buffer.h>


namespace mqas::tools
{
    void datagram_buffer::push(const std::span<uint8_t>& buf)
    {
        if(_split_queue.empty())
        {
            _write_pos = 0;
            _read_pos = 0;
        }
        auto size = _write_pos + buf.size();
        if(_buffer.size() < size)
            _buffer.resize(size);
        std::memcpy(_buffer.data() + _write_pos,buf.data(),buf.size());
        _write_pos += buf.size();
        _split_queue.push(buf.size());
    }

    std::optional<std::span<uint8_t>> datagram_buffer::pop()
    {
        if(_split_queue.empty())
            return {};
        auto size = _split_queue.front();
        std::span<uint8_t> ret(_buffer.data() + _read_pos, size);
        _read_pos += size;
        _split_queue.pop();
        return ret;
    }

    size_t datagram_buffer::count() const
    {
        return _split_queue.size();
    }

    bool datagram_buffer::empty() const
    {
        return _split_queue.empty();
    }
} // namespace mqas::tools





