#pragma once

#include <portaudio.h>
#include <sigc++/sigc++.h>
#include <span>
#include <atomic>
#include <mutex>
#include <opus/opus.h>
#include <speex/speex_preprocess.h>
#include <speex/speex_echo.h>
#include <array>

class AudioStream
{
public:
    AudioStream();
    ~AudioStream();

    bool start(int sample_rate = 48000, int channels = 1, int frame_size = 480, int max_packet_size = 4000,
         int noise_suppress = -30);
    void close();
    
    sigc::connection reg_on_record_callback(sigc::slot<void(const std::span<uint8_t>&,uint16_t)> callback);
    void unreg_on_record_callback(sigc::connection conn);

    size_t on_receive_data(const std::span<uint8_t>& data);
    size_t on_receive_data_def(const std::span<uint8_t>& data);
private:
    static int port_audio_callback_static(const void* inputBuffer, void* outputBuffer,
                               unsigned long framesPerBuffer,
                               const PaStreamCallbackTimeInfo* timeInfo,
                               PaStreamCallbackFlags statusFlags,
                               void* userData);
    int port_audio_callback(const void* inputBuffer, void* outputBuffer,
                                unsigned long framesPerBuffer,
                                const PaStreamCallbackTimeInfo* timeInfo,
                                PaStreamCallbackFlags statusFlags);

private:
    sigc::signal<void(const std::span<uint8_t>&,uint16_t)> on_record_signal;
    std::mutex on_record_mutex;
    std::atomic<bool> is_start = false;
    //decoder encoder
    OpusEncoder* encoder = nullptr;
    OpusDecoder* decoder = nullptr;
    //Speex noise suppression
    SpeexPreprocessState* preprocess_state = nullptr;
    //Speex echo suppression
    SpeexEchoState* echo_state = nullptr;
    //stream
    PaStream* stream = nullptr;
    //config
    int sample_rate     = 48000;
    int channels        = 1;
    int frame_size      = 480; 
    int max_packet_size = 4000;
    int noise_suppress = -30;
    //buffer
    std::vector<int16_t> decode_buffer;
    std::vector<int16_t> processed_buffer;
    //swap buffer
    std::array<std::vector<uint8_t>, 2> record_buffer;
    std::array<std::vector<int16_t>, 2> far_end_buffer;
    //swap index
    std::atomic<int> swap_index_record = 0;
    std::atomic<int> swap_index_far_end = 0;
};