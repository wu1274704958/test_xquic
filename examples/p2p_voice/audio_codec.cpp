#include "audio_codec.h"
#include <easylogging++.h>
#include <mqas/comm/binary.hpp>
#include <zlib.h>

#define USE_OPUS 1


audio_codec::audio_codec()
{
}

audio_codec::~audio_codec()
{
    close();
}

const char* audio_codec::strerror(int error_code)
{
	return opus_strerror(error_code);
}

int audio_codec::init(int sample_rate, int channels,int frame_size, int application,int jitter_buf_size)
{
    _frame_size = frame_size;
	_sample_rate = sample_rate;
	_channels = channels;
    int opus_err = 0;
    bool is_err = false;
    #if USE_OPUS
    _encoder = opus_encoder_create(sample_rate, channels, application, &opus_err);
    if (opus_err != OPUS_OK)
    {
        is_err = true;
        CLOG(ERROR, "audio") << "opus_encoder_create failed: " << opus_strerror(opus_err);
        goto END;
    }
    _decoder = opus_decoder_create(sample_rate, channels, &opus_err);
    if (opus_err != OPUS_OK)
    {
        is_err = true;
        CLOG(ERROR, "audio") << "opus_decoder_create failed: " << opus_strerror(opus_err);
        goto END;
    }
    #endif
    END:
    if(is_err)
    {
        close();
        return opus_err;
    }
    _jitter_max_count = jitter_buf_size * 2;
    _jitter_half_count = jitter_buf_size;

    _jitter_buffer.resize(_jitter_max_count * _frame_size * channels,0);
    _cached_index_buf.resize(_jitter_max_count,0);
    #if JITTER_BUF_CHECKSUM
    _jitter_checksum_buf.resize(_jitter_max_count,0);
    #endif
    #if USE_OPUS
    set_fec(true);
    #endif
    _initialized.store(true,std::memory_order::memory_order_release);
	return opus_err;
}

void audio_codec::close()
{
    if (_encoder)
        opus_encoder_destroy(_encoder);
    if (_decoder)
        opus_decoder_destroy(_decoder);
    _encoder = nullptr;
    _decoder = nullptr;
    _initialized.store(false,std::memory_order::memory_order_release);
}

int audio_codec::encode(const std::span<int16_t>& in, int frame_size, std::span<uint8_t>& out)
{
    if(out.size() < HEADER_SIZE + (frame_size * sizeof(int16_t) * _channels))
        return OPUS_BUFFER_TOO_SMALL;
    #if USE_OPUS
	int byte_size = opus_encode(_encoder, in.data(), frame_size, out.data() + HEADER_SIZE, out.size() - HEADER_SIZE);
	if (byte_size < 0)
	{
		CLOG(ERROR, "audio") << "Opus encode failed: " << opus_strerror(byte_size);
		return byte_size;
	}
    #else
    int byte_size = in.size() * sizeof(int16_t);
    std::memcpy(out.data() + HEADER_SIZE,in.data(),byte_size);
    #endif
    mqas::comm::to_big_endian(++_send_index,out);
    uint16_t crc = static_cast<uint16_t>(crc32(0, out.data() + HEADER_SIZE, byte_size));
    mqas::comm::to_big_endian(crc, out, sizeof(uint32_t));
    #if !NDEBUG
    CLOG(ERROR, "audio") << "send index : " << _send_index;
    #endif
	return byte_size + HEADER_SIZE;
}

int audio_codec::decode(const std::span<uint8_t>& in, int frame_size)
{
    auto recv_base_index = _recv_base_index.load(std::memory_order::memory_order_acquire);
    if(in.size() < HEADER_SIZE)
		return OPUS_BAD_ARG;
    auto checksum = static_cast<uint16_t>(crc32(0, in.data() + HEADER_SIZE, in.size() - HEADER_SIZE));
    auto recv_checksum = mqas::comm::from_big_endian<uint16_t>(in,sizeof(uint32_t));
    auto index = mqas::comm::from_big_endian<uint32_t>(in);

    //todo: check is previous index    
    if (checksum != recv_checksum)
    {
        CLOG(ERROR, "audio") << "audio_codec decode checksum failed index = " << index << " checksum = " << checksum << " size = " << in.size() - HEADER_SIZE;
		return OPUS_OK;
    }

    if (recv_base_index > 0 &&  index < (recv_base_index * _jitter_max_count) - _jitter_half_count )
    {
        CLOG(ERROR, "audio") << "audio_codec decode recv previous index. index = " << index << " current =  " << recv_base_index  << " checksum = " << checksum << " size = " << in.size() - HEADER_SIZE;
		return OPUS_OK;
    }
    auto recv_count = _recv_count.load(std::memory_order::memory_order_acquire) + 1;
    std::span<int16_t> out = try_get_jitter_buffer(index);
    
    #if USE_OPUS

    int byte_size = opus_decode(_decoder, in.data() + HEADER_SIZE, in.size() - HEADER_SIZE, out.data(), frame_size, 0);
    if (byte_size < 0) {
        #if !NDEBUG
        CLOG(ERROR, "audio") << "Opus decode failed: " << opus_strerror(byte_size);
        #endif
    }

    if(byte_size <= 0)
        byte_size = opus_decode(_decoder, nullptr, 0, out.data(), frame_size, 1);
    #else
    int byte_size = (in.size() - HEADER_SIZE) / sizeof(int16_t);
    std::memcpy(out.data(),in.data() + HEADER_SIZE,byte_size * sizeof(int16_t));
    #endif
    
    #if JITTER_BUF_CHECKSUM
    if(byte_size > 0)
    {
        const std::lock_guard<std::mutex> lock(_jitter_using);
        _jitter_checksum_buf[get_cache_index_offset(index)] = crc32(0,(uint8_t*)out.data(),frame_size * _channels * sizeof(uint16_t));
    }
    #endif
    #if !NDEBUG
        CLOG(DEBUG, "audio") << "decode new index: " << index;
    #endif
    if(index / _jitter_max_count > recv_base_index )
    {
        recv_base_index = index / _jitter_max_count;
        #if !NDEBUG
            CLOG(DEBUG, "audio") << "base index change to " << recv_base_index;
        #endif
        _recv_base_index.store(recv_base_index,std::memory_order::memory_order_release);
    }

    _recv_count.store(recv_count,std::memory_order::memory_order_release);
	return byte_size;
}

void audio_codec::set_fec(bool enable) {
    _fec_enabled = enable;
    opus_encoder_ctl(_encoder, OPUS_SET_INBAND_FEC(enable ? 1 : 0));
    opus_decoder_ctl(_decoder, OPUS_SET_PACKET_LOSS_PERC(enable ? 35 : 15));
}

std::pair<int,uint32_t> audio_codec::next_far_end_data(std::span<int16_t>& out,int frame_size)
{ 
    const int size = _frame_size * _channels * sizeof(int16_t);
    auto recv_count = _recv_count.load(std::memory_order::memory_order_acquire);
    if(recv_count < _jitter_half_count)
    {
        #if USE_OPUS
        auto byte_size = opus_decode(_decoder, nullptr, 0, out.data(), frame_size, 1);
        #else
        auto byte_size = 0;
        #endif
        if(byte_size <= 0)
            std::memset(out.data(),0,size);
        return {byte_size,0};
    }
    auto played_index = _played_index.load(std::memory_order::memory_order_acquire) + 1;
   
    const int offset = get_jitter_buf_offset(played_index);
    
    _jitter_using.lock();

    if(_cached_index_buf[get_cache_index_offset(played_index)] == played_index)
    {
        memcpy(out.data(),_jitter_buffer.data() + offset,size);
        #if JITTER_BUF_CHECKSUM
        if(_jitter_checksum_buf[get_cache_index_offset(played_index)] != crc32(0,(uint8_t*)(_jitter_buffer.data() + offset),size))
            CLOG(DEBUG, "audio") << "curr = " << played_index << " jitter checksum failed ";
        #endif
        _jitter_using.unlock();
    }else{
        _jitter_using.unlock();
        #if USE_OPUS
        auto byte_size = opus_decode(_decoder, nullptr, 0, out.data(), frame_size, 1);
        #else
        auto byte_size = 0;
        #endif
        if(byte_size <= 0)
            std::memset(out.data(),0,size);
        ++_loses_pack_num;
        CLOG(DEBUG, "audio") << "curr = " << played_index << " total " << " lose " << _loses_pack_num << " pack. lose rate " 
             << _loses_pack_num / (float)played_index;
    }

    _played_index.store(played_index,std::memory_order::memory_order_release);
    
	return { size , played_index};
}

bool audio_codec::initialized() const
{
    return _initialized.load(std::memory_order::memory_order_acquire);
}

std::span<int16_t> audio_codec::try_get_jitter_buffer(uint32_t index)
{
    const std::lock_guard<std::mutex> lock(_jitter_using);
    const int frame_size = _frame_size * _channels;
    const int offset = (index % _jitter_max_count) * frame_size;
    _cached_index_buf[index % _jitter_max_count] = index;
    return std::span<int16_t>( _jitter_buffer.data() + offset, frame_size );
}

size_t audio_codec::get_cache_index_offset(uint32_t index) const
{
    return index % _jitter_max_count;
}

size_t audio_codec::get_jitter_buf_offset(uint32_t index) const
{
    const int size = _frame_size * _channels;
    return (index % _jitter_max_count) * size;
}