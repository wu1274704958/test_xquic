#pragma once

#include <portaudio.h>
#include <sigc++/sigc++.h>
#include <span>
#include <atomic>

class AudioStream
{
public:
    AudioStream();
    ~AudioStream();

    void start();
    void stop();
    
    sigc::connection reg_on_record_callback(sigc::slot<void(const std::span<uint8_t>&)> callback);
    void unreg_on_record_callback(sigc::connection conn);

    size_t on_receive_data(const std::span<uint8_t>& data);

private:
    sigc::signal<void(const std::span<uint8_t>&)> on_record_signal;
    std::atomic<bool> on_record_signal_blocked = false;
    
};