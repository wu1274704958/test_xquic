#pragma once

#define USE_SPEEX 0
#define USE_WEBRTC 1

#include <portaudio.h>
#include <sigc++/sigc++.h>
#include <span>
#include <atomic>
#include <mutex>
#include <opus/opus.h>
#if USE_SPEEX
#include <speex/speex_preprocess.h>
#include <speex/speex_echo.h>
#endif
#include <array>
#include <mqas/io/idle.h>
#include "audio_codec.h"
#include <mqas/tools/datagram_buffer.h>

#if USE_WEBRTC
#include <api/scoped_refptr.h>
#include <modules/audio_processing/include/audio_processing.h>
#endif

class AudioStream
{
public:
    AudioStream();
    ~AudioStream();

    bool start(mqas::io::Context* io_cxt,int sample_rate = 48000, int channels = 1, int frame_size = 480, int max_packet_size = 4000,
         int noise_suppress = -30,int jitter_buf_size = 12);
    void close();

    //always used on main thread
    sigc::signal<void(const std::span<int16_t>&)> on_decode_far_end_data;
    
    sigc::connection reg_on_record_callback(sigc::slot<void(const std::span<uint8_t>&,uint16_t)> callback);
    void unreg_on_record_callback(sigc::connection conn);

    void on_receive_data(const std::span<uint8_t>& data);
    void on_receive_data_def(const std::span<uint8_t>& data);
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
    void emit_idle_callback(mqas::io::Idle* idle);
    void on_receive_data_internal(const std::span<uint8_t>& data,int frame_size);

private:
    sigc::signal<void(const std::span<uint8_t>&,uint16_t)> on_record_signal;
    std::mutex on_record_mutex;
    std::atomic<bool> is_start = false;
    //decoder encoder
    audio_codec _codec;
    #if USE_SPEEX
    //Speex noise suppression
    SpeexPreprocessState* preprocess_state = nullptr;
    //Speex echo suppression
    SpeexEchoState* echo_state = nullptr;
    #endif
    #if USE_WEBRTC
    webrtc::scoped_refptr<webrtc::AudioProcessing> audio_processing;
    webrtc::StreamConfig stream_config;
    #endif
    //stream
    PaStream* stream = nullptr;
    //config
    int sample_rate     = 48000;
    int channels        = 1;
    int frame_size      = 480; 
    int max_packet_size = 4000;
    int noise_suppress = -30;
    //buffer
    std::vector<int16_t> processed_buffer;
    std::vector<int16_t> last_play_buffer;
    //record buffer
    std::vector<uint8_t> record_buffer;
    mqas::tools::datagram_buffer record_datagram_buffer;
    std::atomic_uint32_t _record_count = 0;
    std::mutex record_mutex;

    mqas::io::Context* io_cxt;
    //record emit timer run on main thread
    std::shared_ptr<mqas::io::Idle> idle;
};