#pragma once
#include <span>
#include <cstdint>
#include <opus/opus.h>
#include <atomic>
#include <mutex>
#include <functional>

struct audio_codec
{                                         //index + checksum
    static constexpr size_t HEADER_SIZE = sizeof(uint32_t) + sizeof(uint16_t);
    audio_codec();
    ~audio_codec();
    const char* strerror(int error_code);
    int init(int sample_rate, int channels,int frame_size,int application,int jitter_buf_size = 12);
    void close();
    int encode(const std::span<int16_t>& in,int frame_size,std::span<uint8_t>& out);
    int decode(const std::span<uint8_t>& in, int frame_size);
    std::pair<int,uint32_t> next_far_end_data(std::span<int16_t>& out,int frame_size);
private:
    std::span<int16_t> try_get_jitter_buffer(uint32_t index);
    void set_fec(bool enable); 
    inline size_t get_cache_index_offset(uint32_t index) const;
    inline size_t get_jitter_buf_offset(uint32_t index) const;
private:
    int _sample_rate;
    int _channels;
    int _frame_size;
    OpusEncoder* _encoder = nullptr;
    OpusDecoder* _decoder = nullptr;
    std::atomic_uint32_t _send_index = 0;
    std::atomic_uint32_t _played_index = 0;
    std::atomic_uint32_t _recv_base_index = 0;
    std::atomic_uint32_t _recv_count = 0;
    std::vector<int16_t> _jitter_buffer;
    std::vector<uint32_t> _cached_index;
    std::mutex _jitter_using;
    std::atomic_uint8_t _jitter_max_count = 10;
    std::atomic_uint8_t _jitter_half_count = 5;
    std::atomic_uint32_t _loses_pack_num = 0;
    //std::atomic_uint8_t _jitter_count = 0;
    bool _fec_enabled:1 = false;
};
