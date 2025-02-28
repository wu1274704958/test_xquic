#include <iostream>
#include <cstdlib>
#include <cstring>
#include <portaudio.h>
#include <opus/opus.h>
#include <speex/speex_echo.h>
#include <speex/speex_preprocess.h>


constexpr int SAMPLE_RATE     = 48000;
constexpr int CHANNELS        = 1;
constexpr int FRAME_SIZE      = 480;       // 每帧 10ms 数据
constexpr int MAX_PACKET_SIZE = 4000;      // Opus 编码后数据最大长度

// 用于传递到回调函数的上下文数据
struct AudioData {
    OpusEncoder* encoder;
    OpusDecoder* decoder;
    unsigned char opusBuffer[MAX_PACKET_SIZE];

    SpeexEchoState* echo_state;           // Speex 回声消除状态
    SpeexPreprocessState* preprocess_state; // Speex 噪声抑制状态

    int16_t far_end_buffer[FRAME_SIZE];     // 存储上一帧的播放数据作为远端参考
    int16_t processed_buffer[FRAME_SIZE];   // 存储回声消除+噪声抑制后的数据
};

// 回调函数：对采集到的音频数据做回声消除、噪声抑制，再进行 Opus 编解码处理
static int audioCallback(const void* inputBuffer, void* outputBuffer,
                         unsigned long framesPerBuffer,
                         const PaStreamCallbackTimeInfo* /*timeInfo*/,
                         PaStreamCallbackFlags /*statusFlags*/,
                         void* userData) {
    auto* data = static_cast<AudioData*>(userData);
    const int16_t* in = static_cast<const int16_t*>(inputBuffer);
    int16_t* out = static_cast<int16_t*>(outputBuffer);

    // 如果输入为空，则输出静音数据
    if (!in) {
        std::memset(out, 0, framesPerBuffer * CHANNELS * sizeof(int16_t));
        return paContinue;
    }

    // 1. 回声消除
    // 使用当前麦克风输入 in，远端参考信号 data->far_end_buffer（上一帧的播放数据），
    // 得到回声消除后的数据存放到 data->processed_buffer 中
    speex_echo_cancellation(data->echo_state, in, data->far_end_buffer, data->processed_buffer);

    // 2. 噪声抑制（噪声抑制默认开启，可通过 preprocess_ctl 调整参数）
    speex_preprocess_run(data->preprocess_state, data->processed_buffer);

    // 3. 使用处理后的数据进行 Opus 编解码
    int nbBytes = opus_encode(data->encoder, data->processed_buffer, framesPerBuffer,
                              data->opusBuffer, MAX_PACKET_SIZE);
    if (nbBytes < 0) {
        std::cerr << "Opus 编码错误: " << opus_strerror(nbBytes) << std::endl;
        std::memset(out, 0, framesPerBuffer * CHANNELS * sizeof(int16_t));
        return paContinue;
    }

    int frame_size = opus_decode(data->decoder, data->opusBuffer, nbBytes,
                                 out, framesPerBuffer, 0);
    if (frame_size < 0) {
        std::cerr << "Opus 解码错误: " << opus_strerror(frame_size) << std::endl;
        std::memset(out, 0, framesPerBuffer * CHANNELS * sizeof(int16_t));
        return paContinue;
    }

    // 若解码出的帧数不足，补 0
    if (static_cast<unsigned>(frame_size) < framesPerBuffer) {
        std::memset(out + frame_size * CHANNELS, 0,
                    (framesPerBuffer - frame_size) * CHANNELS * sizeof(int16_t));
    }

    // 4. 更新远端参考信号：将本次输出数据保存，用于下一帧的回声消除参考
    std::memcpy(data->far_end_buffer, out, framesPerBuffer * CHANNELS * sizeof(int16_t));

    return paContinue;
}

extern "C" {
typedef void (*PaUtilLogCallback ) (const char *log);
extern void PaUtil_SetDebugPrintFunction(PaUtilLogCallback  cb);
}

void PaLogCallback(const char *log)
{
    //printf("%s\n", log);
}

int main2() {
    // 初始化 PortAudio
    PaUtil_SetDebugPrintFunction(PaLogCallback);
    PaError pa_err = Pa_Initialize();


    if (pa_err != paNoError) {
        std::cerr << "PortAudio 初始化失败: " << Pa_GetErrorText(pa_err) << std::endl;
        return 1;
    }

    // 创建 Opus 编码器
    int opus_err;
    OpusEncoder* encoder = opus_encoder_create(SAMPLE_RATE, CHANNELS,
                                               OPUS_APPLICATION_VOIP, &opus_err);
    if (opus_err != OPUS_OK) {
        std::cerr << "创建 Opus 编码器失败: " << opus_strerror(opus_err) << std::endl;
        Pa_Terminate();
        return 1;
    }

    // 创建 Opus 解码器
    OpusDecoder* decoder = opus_decoder_create(SAMPLE_RATE, CHANNELS, &opus_err);
    if (opus_err != OPUS_OK) {
        std::cerr << "创建 Opus 解码器失败: " << opus_strerror(opus_err) << std::endl;
        opus_encoder_destroy(encoder);
        Pa_Terminate();
        return 1;
    }

    // 初始化 Speex 回声消除状态
    // filter_length 表示滤波器长度（回声尾部长度），可根据实际需要调整
    int filter_length = FRAME_SIZE; // 此处示例设为与帧大小相同
    SpeexEchoState* echo_state = speex_echo_state_init(FRAME_SIZE, filter_length);
    if (!echo_state) {
        std::cerr << "初始化 Speex 回声消除状态失败" << std::endl;
        opus_encoder_destroy(encoder);
        opus_decoder_destroy(decoder);
        Pa_Terminate();
        return 1;
    }

    // 初始化 Speex 噪声抑制状态
    SpeexPreprocessState* preprocess_state = speex_preprocess_state_init(FRAME_SIZE, SAMPLE_RATE);
    if (!preprocess_state) {
        std::cerr << "初始化 Speex 噪声抑制状态失败" << std::endl;
        speex_echo_state_destroy(echo_state);
        opus_encoder_destroy(encoder);
        opus_decoder_destroy(decoder);
        Pa_Terminate();
        return 1;
    }
    // 设置噪声抑制参数，例如将噪声抑制量设为 -30 dB
    int noiseSuppress = -30;
    speex_preprocess_ctl(preprocess_state, SPEEX_PREPROCESS_SET_NOISE_SUPPRESS, &noiseSuppress);

    // 构造传递给回调函数的上下文数据
    AudioData audioData;
    audioData.encoder = encoder;
    audioData.decoder = decoder;
    audioData.echo_state = echo_state;
    audioData.preprocess_state = preprocess_state;
    std::memset(audioData.far_end_buffer, 0, sizeof(audioData.far_end_buffer));
    std::memset(audioData.opusBuffer, 0, sizeof(audioData.opusBuffer));
    std::memset(audioData.processed_buffer, 0, sizeof(audioData.processed_buffer));

    // 打开全双工流（同时支持输入和输出）
    PaStream* stream;
    pa_err = Pa_OpenDefaultStream(&stream,
                                  CHANNELS,   // 输入通道数
                                  CHANNELS,   // 输出通道数
                                  paInt16,    // 16位整数格式
                                  SAMPLE_RATE,
                                  FRAME_SIZE, // 每帧样本数
                                  audioCallback,
                                  &audioData);
    if (pa_err != paNoError) {
        std::cerr << "打开流失败: " << Pa_GetErrorText(pa_err) << std::endl;
        speex_echo_state_destroy(echo_state);
        speex_preprocess_state_destroy(preprocess_state);
        opus_encoder_destroy(encoder);
        opus_decoder_destroy(decoder);
        Pa_Terminate();
        return 1;
    }

    // 启动流
    pa_err = Pa_StartStream(stream);
    if (pa_err != paNoError) {
        std::cerr << "启动流失败: " << Pa_GetErrorText(pa_err) << std::endl;
        Pa_CloseStream(stream);
        speex_echo_state_destroy(echo_state);
        speex_preprocess_state_destroy(preprocess_state);
        opus_encoder_destroy(encoder);
        opus_decoder_destroy(decoder);
        Pa_Terminate();
        return 1;
    }

    std::cout << "................." << std::endl;
    std::cin.get();

    pa_err = Pa_StopStream(stream);
    if (pa_err != paNoError) {
        std::cerr << "停止流失败: " << Pa_GetErrorText(pa_err) << std::endl;
    }
    Pa_CloseStream(stream);
    Pa_Terminate();

    // 清理 Speex 和 Opus 资源
    speex_echo_state_destroy(echo_state);
    speex_preprocess_state_destroy(preprocess_state);
    opus_encoder_destroy(encoder);
    opus_decoder_destroy(decoder);

    return 0;
}
