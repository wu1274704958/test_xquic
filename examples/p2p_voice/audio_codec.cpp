#include "audio_codec.h"
#include <easylogging++.h>
#include <mqas/comm/binary.hpp>
#include <zlib.h>

#define DEBUG 0

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

int audio_codec::init(int sample_rate, int channels, int application)
{
	_sample_rate = sample_rate;
	_channels = channels;
    int opus_err = 0;
    bool is_err = false;
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
    END:
    if(is_err)
        close();

    set_fec(true);
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
}

int audio_codec::encode(const std::span<int16_t>& in, int frame_size, std::span<uint8_t>& out)
{
    if(out.size() < HEADER_SIZE + (frame_size * sizeof(int16_t) * _channels))
        return OPUS_BUFFER_TOO_SMALL;

	int byte_size = opus_encode(_encoder, in.data(), frame_size, out.data() + HEADER_SIZE, out.size() - HEADER_SIZE);
	if (byte_size < 0)
	{
		CLOG(ERROR, "audio") << "Opus encode failed: " << opus_strerror(byte_size);
		return byte_size;
	}
    mqas::comm::to_big_endian(++_send_index,out);
    uint16_t crc = static_cast<uint16_t>(crc32(0, out.data() + HEADER_SIZE, byte_size));
    mqas::comm::to_big_endian(crc, out, sizeof(uint32_t));
#if DEBUG
    CLOG(INFO, "audio") << "audio_codec encode index = " << _send_index << " checksum = " << crc << " size = " << byte_size;
#endif
	return byte_size + HEADER_SIZE;
}

int audio_codec::decode(const std::span<uint8_t>& in, std::span<int16_t>& out, int frame_size)
{
    if(in.size() < HEADER_SIZE)
		return OPUS_BAD_ARG;
    auto checksum = static_cast<uint16_t>(crc32(0, in.data() + HEADER_SIZE, in.size() - HEADER_SIZE));
    auto recv_checksum = mqas::comm::from_big_endian<uint16_t>(in,sizeof(uint32_t));
    auto index = mqas::comm::from_big_endian<uint32_t>(in);
#if DEBUG
    CLOG(INFO, "audio") << "audio_codec decode index = " << index << " checksum = " << checksum << " x " << recv_checksum << " size = " << in.size() - HEADER_SIZE;
#endif
    //todo: check is previous index    
    if (checksum != recv_checksum || _recv_index >= index)
    {
        CLOG(ERROR, "audio") << "audio_codec decode checksum failed index = " << index << " checksum = " << checksum << " size = " << in.size() - HEADER_SIZE;
		return OPUS_OK;
    }
    
    _recv_index = index;

    int byte_size = opus_decode(_decoder, in.data() + HEADER_SIZE, in.size() - HEADER_SIZE, out.data(), frame_size, 0);
    if (byte_size < 0) {
        CLOG(ERROR, "audio") << "Opus decode failed: " << opus_strerror(byte_size);
    }
	return byte_size;
}

void audio_codec::set_fec(bool enable) {
    _fec_enabled = enable;
    opus_encoder_ctl(_encoder, OPUS_SET_INBAND_FEC(enable ? 1 : 0));
    opus_decoder_ctl(_decoder, OPUS_SET_PACKET_LOSS_PERC(enable ? 35 : 15));
}

int audio_codec::forward_prediction(std::span<int16_t>& out,int frame_size)
{
    if(!_fec_enabled)
        return 0;
    int byte_size = opus_decode(_decoder, nullptr, 0, out.data(), frame_size, 1);
    if (byte_size < 0) {
        CLOG(ERROR, "audio") << "Opus forward prediction failed: " << opus_strerror(byte_size);
    }
	return byte_size;
}
