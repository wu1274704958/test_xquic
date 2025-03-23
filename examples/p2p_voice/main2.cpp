#include <iostream>
#include "AudioStream.h"
#include "PortAudioGlobal.h"

constexpr int SAMPLE_RATE     = 48000;
constexpr int CHANNELS        = 1;
constexpr int FRAME_SIZE      = 480;       // 每帧 10ms 数据
constexpr int MAX_PACKET_SIZE = 4000;      // Opus 编码后数据最大长度


int main2() {

    PortAudioGlobal port_audio_global("audio_log_conf.txt");
    mqas::io::Context io_cxt;
    std::atomic<bool> is_running = true;
    AudioStream audio_stream;

    audio_stream.start(&io_cxt,SAMPLE_RATE, CHANNELS, FRAME_SIZE, MAX_PACKET_SIZE, -30);
    audio_stream.reg_on_record_callback([&audio_stream](const std::span<uint8_t>& data, uint16_t framesPerBuffer) {
        audio_stream.on_receive_data_def(data);
    });

    std::cout << "................." << std::endl;

    io_cxt.run_until(is_running);

    audio_stream.close();


    return 0;
}
