#include "AudioStream.h"
#include "easylogging++.h"
#include <mqas/comm/binary.hpp>
#include <speex/speex_echo.h>

#define USE_OPUS 1

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
    PaError pa_err = paNoError;
#if USE_OPUS
    int opus_err = 0;
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
#endif
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
        processed_buffer.resize(frame_size * channels, 0);
        last_play_buffer.resize(frame_size * channels, 0);
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
    auto swap_index = get_other_index(swap_index_record.load(std::memory_order_acquire));
    if(last_record_index != swap_index)
    {
        on_record_signal.emit(std::span<uint8_t>(record_buffer[swap_index].data(), last_record_size), last_record_frames);
        last_record_index = swap_index;
    }
}

int AudioStream::port_audio_callback(const void* inputBuffer, void* outputBuffer,
                                unsigned long framesPerBuffer,
                                const PaStreamCallbackTimeInfo* timeInfo,
                                PaStreamCallbackFlags statusFlags)
{
    //test
    // if(!inputBuffer)
    //     return paContinue;
    // std::memcpy((void*)outputBuffer, inputBuffer, framesPerBuffer * channels * sizeof(int16_t));
    // return paContinue;
    //process far end
    const size_t byte_size = framesPerBuffer * channels * sizeof(int16_t);

    auto swap_index = swap_index_far_end.load(std::memory_order_acquire);
    std::memcpy((void*)outputBuffer, far_end_buffer[swap_index].data(), byte_size);
    //process record
    if(!inputBuffer)
        return paContinue;
    auto swap_index_for_record = swap_index_record.load(std::memory_order_acquire);
    const int16_t* in = static_cast<const int16_t*>(inputBuffer);
    speex_echo_cancellation(echo_state, in, last_play_buffer.data(), processed_buffer.data());

    speex_preprocess_run(preprocess_state, processed_buffer.data());
   
    auto& record_buf = record_buffer[swap_index_for_record];

#if USE_OPUS
    last_record_size = opus_encode(encoder, processed_buffer.data(), framesPerBuffer, record_buf.data(), record_buf.size());
    if(last_record_size < 0)
    {
        CLOG(ERROR,"audio") << "Opus encode failed: " << opus_strerror(last_record_size);
        return paContinue;
    }
#else
    last_record_size = byte_size;
    std::memcpy(record_buf.data(),processed_buffer.data(),byte_size);
#endif

    last_record_frames = framesPerBuffer;
    swap_index_record.store(get_other_index(swap_index_for_record), std::memory_order_release);

    //copy to last play buffer
    std::memcpy(last_play_buffer.data(),outputBuffer,byte_size);

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

void AudioStream::on_receive_data_internal(const std::span<uint8_t>& data,int cur_frame_size,int offset)
{
    //other
    auto swap_index = get_other_index(swap_index_far_end.load(std::memory_order_acquire));
    
    auto& dst_buffer = far_end_buffer[swap_index];
    //decode
#if USE_OPUS
    int byte_size = opus_decode(decoder, data.data() + offset, data.size() - offset, dst_buffer.data(), cur_frame_size, 0);
    if (byte_size < 0) {
        CLOG(ERROR,"audio") << "Opus decode failed: " << opus_strerror(byte_size);
        return;
    }
#else
    const size_t byte_size = cur_frame_size * channels * sizeof(int16_t);
    std::memcpy(dst_buffer.data(), data.data() + offset, byte_size);
#endif
   
    on_decode_far_end_data.emit(std::span<int16_t>(dst_buffer.data(),byte_size));

    //swap index
    swap_index_far_end.store(swap_index, std::memory_order_release);
}

void AudioStream::on_receive_data(const std::span<uint8_t>& data)
{
    const uint16_t cur_frame_size = mqas::comm::from_big_endian<uint16_t>(data);
    constexpr int offset = sizeof(uint16_t);

    on_receive_data_internal(data,cur_frame_size,offset);
}

void AudioStream::on_receive_data_def(const std::span<uint8_t>& data)
{
    on_receive_data_internal(data,frame_size,0);
}