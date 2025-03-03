#pragma once
#include <mqas/core/stream.h>
#include <sigc++/sigc++.h>
#include "AudioStream.h"

class VoiceStream : public mqas::core::IStreamVariant
{
    public:
    VoiceStream(){}
    ~VoiceStream();
    sigc::signal<void(bool)> on_connected_signal;
    mqas::core::StreamVariantErrcode on_change(const std::span<uint8_t> &params,
        std::vector<uint8_t> &ret_buf);
    mqas::core::StreamVariantErrcode on_local_change(const std::span<uint8_t>& params,
         std::vector<uint8_t>& ret_buf);
    
    void on_peer_change_ret(mqas::core::StreamVariantErrcode code,const std::span<uint8_t>& params);
    private:
    bool init_audio_stream();
    void close_audio_stream();
    void on_record_data(const std::span<uint8_t>& data, uint16_t framesPerBuffer);
    private:
    AudioStream audio_stream;
    sigc::connection on_recv_connect;
    sigc::connection on_record_conn;
    bool is_audio_stream_init:1 = false;
    std::atomic<bool> is_ready = false;
};
