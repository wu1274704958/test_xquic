#pragma once
#include <span>
#include <cstdint>
#include <opus/opus.h>
#include <atomic>

struct audio_codec
{                                         //index + checksum
    static constexpr size_t HEADER_SIZE = sizeof(uint32_t) + sizeof(uint16_t);
    audio_codec();
    ~audio_codec();
    const char* strerror(int error_code);
    int init(int sample_rate, int channels,int application);
    void close();
    int encode(const std::span<int16_t>& in,int frame_size,std::span<uint8_t>& out);
    int decode(const std::span<uint8_t>& in,std::span<int16_t>& out, int frame_size);
    int forward_prediction(std::span<int16_t>& out,int frame_size);
private:
    void set_fec(bool enable); 
private:
    int _sample_rate;
    int _channels;
    OpusEncoder* _encoder = nullptr;
    OpusDecoder* _decoder = nullptr;
    std::atomic_uint32_t _send_index = 0;
    std::atomic_uint32_t _recv_index = 0;
    bool _fec_enabled:1 = false;
};
