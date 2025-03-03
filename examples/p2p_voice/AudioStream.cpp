#include "AudioStream.h"
#include "easylogging++.h"
#include <mqas/comm/binary.hpp>
#include <speex/speex_echo.h>

AudioStream::AudioStream()
{
    
}

AudioStream::~AudioStream()
{
    close();
}

bool AudioStream::start(mqas::io::Context* io_cxt,int sample_rate, int channels, int frame_size, int max_packet_size, int noise_suppress)
{
    if(io_cxt == nullptr)
        return false;
    if(is_start.load(std::memory_order_acquire))
        return false;
    this->io_cxt = io_cxt;
    this->sample_rate = sample_rate;
    this->channels = channels;
    this->frame_size = frame_size;
    this->max_packet_size = max_packet_size;
    bool is_err = false;
    //init decoder encoder
    int opus_err = 0;
    PaError pa_err = paNoError;
    encoder = opus_encoder_create(sample_rate, channels, OPUS_APPLICATION_VOIP, &opus_err);
    if(opus_err != OPUS_OK)
    {
        is_err = true;
        CLOG(ERROR,"audio") << "opus_encoder_create failed: " << opus_strerror(opus_err);
        goto END;
    }
    decoder = opus_decoder_create(sample_rate, channels, &opus_err);
    if(opus_err != OPUS_OK)
    {
        is_err = true;
        CLOG(ERROR,"audio") << "opus_decoder_create failed: " << opus_strerror(opus_err);
        goto END;
    }
    //init Speex noise suppression
    preprocess_state = speex_preprocess_state_init(frame_size * channels, sample_rate);
    if(preprocess_state == nullptr)
    {
        is_err = true;
        CLOG(ERROR,"audio") << "speex_preprocess_state_init failed";
        goto END;
    }
    //init Speex echo cancellation
    speex_preprocess_ctl(preprocess_state, SPEEX_PREPROCESS_SET_NOISE_SUPPRESS, &noise_suppress);

    echo_state = speex_echo_state_init(frame_size * channels, frame_size * channels);
    if(echo_state == nullptr)
    {
        is_err = true;
        CLOG(ERROR,"audio") << "speex_echo_state_init failed";
        goto END;
    }

    //init PortAudio stream
    pa_err = Pa_OpenDefaultStream(&stream, channels, channels, paInt16, sample_rate, frame_size, port_audio_callback_static, this);
    if (pa_err != paNoError) {
        is_err = true;
        CLOG(ERROR,"audio") << "Pa_OpenDefaultStream failed: " << Pa_GetErrorText(pa_err);
        goto END;
    }

    pa_err = Pa_StartStream(stream);
    if (pa_err != paNoError) {
        is_err = true;
        CLOG(ERROR,"audio") << "Pa_StartStream failed: " << Pa_GetErrorText(pa_err);
        goto END;
    }

    if(!is_err)
    {
        decode_buffer.resize(max_packet_size, 0);
        processed_buffer.resize(frame_size * channels, 0);
        for (size_t i = 0; i < record_buffer.size(); i++)
        {
            record_buffer[i].resize(frame_size * sizeof(uint16_t) * channels, 0);
            far_end_buffer[i].resize(frame_size * channels, 0);
        }
    }

    idle = io_cxt->make_shared<mqas::io::Idle>();
    idle->start(std::bind(&AudioStream::emit_idle_callback, this, std::placeholders::_1));

    END:
    is_start.store(!is_err, std::memory_order_release);
    if(is_err)
        close();
    return !is_err;
}

void AudioStream::close()
{
    if(!is_start.load(std::memory_order_acquire))
        return;

    if(encoder)
        opus_encoder_destroy(encoder);
    if(decoder)
        opus_decoder_destroy(decoder);
    if(preprocess_state)
        speex_preprocess_state_destroy(preprocess_state);
    if(echo_state)
        speex_echo_state_destroy(echo_state);
    if (stream)
        Pa_CloseStream(stream);
    if(idle)
        idle.reset();

    is_start.store(false, std::memory_order_release);
}

int AudioStream::port_audio_callback_static(const void* inputBuffer, void* outputBuffer,
                               unsigned long framesPerBuffer,
                               const PaStreamCallbackTimeInfo* timeInfo,
                               PaStreamCallbackFlags statusFlags,
                               void* userData)
{
    return static_cast<AudioStream*>(userData)->port_audio_callback(inputBuffer, outputBuffer, framesPerBuffer, timeInfo, statusFlags);
}

inline int get_other_index(int index)
{
    return index == 0 ? 1 : 0;
}
    
void AudioStream::emit_idle_callback(mqas::io::Idle* idle)
{
    auto swap_index = swap_index_record.load(std::memory_order_acquire);
    if(last_record_index != swap_index)
    {
        on_record_signal.emit(std::span<uint8_t>(record_buffer[get_other_index(swap_index)].data(), last_record_size), last_record_frames);
        last_record_index = swap_index;
    }
}

int AudioStream::port_audio_callback(const void* inputBuffer, void* outputBuffer,
                                unsigned long framesPerBuffer,
                                const PaStreamCallbackTimeInfo* timeInfo,
                                PaStreamCallbackFlags statusFlags)
{
    //process far end
    auto swap_index = swap_index_far_end.load(std::memory_order_acquire);
    std::memcpy((void*)outputBuffer, far_end_buffer[swap_index].data(), framesPerBuffer * channels * sizeof(int16_t));
    //process record
    if(!inputBuffer)
        return paContinue;
    const int16_t* in = static_cast<const int16_t*>(inputBuffer);
    speex_echo_cancellation(echo_state, in, far_end_buffer[swap_index].data(), processed_buffer.data());

    speex_preprocess_run(preprocess_state, processed_buffer.data());
    auto swap_index_for_record = swap_index_record.load(std::memory_order_acquire);

    last_record_size = opus_encode(encoder, processed_buffer.data(), framesPerBuffer, record_buffer[swap_index_for_record].data(), frame_size * sizeof(uint16_t) * channels);
    if(last_record_size < 0)
    {
        CLOG(ERROR,"audio") << "Opus encode failed: " << opus_strerror(last_record_size);
        return paContinue;
    }
    last_record_frames = framesPerBuffer;
    swap_index_record.store(get_other_index(swap_index_for_record), std::memory_order_release);

    return paContinue;
}

sigc::connection AudioStream::reg_on_record_callback(sigc::slot<void(const std::span<uint8_t>&, uint16_t)> callback)
{
    std::lock_guard<std::mutex> lock(on_record_mutex);
    auto conn = on_record_signal.connect(callback);
    return conn;
}

void AudioStream::unreg_on_record_callback(sigc::connection conn)
{
    std::lock_guard<std::mutex> lock(on_record_mutex);
    conn.disconnect();
}

void AudioStream::on_receive_data(const std::span<uint8_t>& data)
{
    uint16_t cur_frame_size = mqas::comm::from_big_endian<uint16_t>(data);
    constexpr int offset = sizeof(uint16_t);
    //decode
    int size = opus_decode(decoder, data.data() + offset, data.size(), decode_buffer.data(), cur_frame_size * channels, 0);
    if (size < 0) {
        CLOG(ERROR,"audio") << "Opus decode failed: " << opus_strerror(size);
        return;
    }

    auto swap_index = swap_index_far_end.load(std::memory_order_acquire);

    //copy data
    far_end_buffer[get_other_index(swap_index)].resize(frame_size * channels, 0);

    const auto data_size = cur_frame_size * sizeof(int16_t) * channels;
    std::memcpy(far_end_buffer[get_other_index(swap_index)].data(), decode_buffer.data(), data_size);

    //swap index
    swap_index_far_end.store(get_other_index(swap_index), std::memory_order_release);
}

void AudioStream::on_receive_data_def(const std::span<uint8_t>& data)
{
    uint16_t cur_frame_size = frame_size;
    constexpr int offset = 0;
    //decode
    int size = opus_decode(decoder, data.data() + offset, data.size(), decode_buffer.data(), cur_frame_size, 0);
    if (size < 0) {
        CLOG(ERROR,"audio") << "Opus decode failed: " << opus_strerror(size);
        return;
    }

    auto swap_index = swap_index_far_end.load(std::memory_order_acquire);

    //copy data
    far_end_buffer[get_other_index(swap_index)].resize(frame_size * channels, 0);

    const auto data_size = cur_frame_size * sizeof(int16_t) * channels;
    std::memcpy(far_end_buffer[get_other_index(swap_index)].data(), decode_buffer.data(), data_size);

    //swap index
    swap_index_far_end.store(get_other_index(swap_index), std::memory_order_release);

}