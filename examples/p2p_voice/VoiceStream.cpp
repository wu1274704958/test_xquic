#include "VoiceStream.h"

#define DEBUG_LOG 0

mqas::core::StreamVariantErrcode VoiceStream::on_change(const std::span<uint8_t> &params,
    std::vector<uint8_t> &ret_buf)
{
    if(!init_audio_stream())
    {
        on_connected_signal.emit(false);
        return mqas::core::StreamVariantErrcode::failed;
    }
    is_ready = true;
    on_connected_signal.emit(true);
    return mqas::core::StreamVariantErrcode::ok;
}
mqas::core::StreamVariantErrcode VoiceStream::on_local_change(const std::span<uint8_t>& params,
    std::vector<uint8_t>& ret_buf)
{
    if(init_audio_stream())
        return mqas::core::StreamVariantErrcode::ok;
    else
    {
        on_connected_signal.emit(false);
        return mqas::core::StreamVariantErrcode::failed;
    }
}
    
void VoiceStream::on_peer_change_ret(mqas::core::StreamVariantErrcode code,const std::span<uint8_t>& params)
{
    if(code == mqas::core::StreamVariantErrcode::ok)
    {
        on_connected_signal.emit(true);
        is_ready = true;
    }else
        on_connected_signal.emit(false);
}

bool VoiceStream::init_audio_stream()
{
    if(is_audio_stream_init)
        return true;
    auto engine = connect_cxt_->engine_cxt_->engine.lock();
    std::shared_ptr<toml::value> config = nullptr;
    if(!engine || !(config = engine->get_config()))
        return false;
    if(!audio_stream.start(&connect_cxt_->engine_cxt_->io_cxt,
        toml::find_or<int>(*config,"audio","sample_rate",48000),
        toml::find_or<int>(*config,"audio","channels",1),
        toml::find_or<int>(*config,"audio","frame_size",480),
        toml::find_or<int>(*config,"audio","max_packet_size",4000),
        toml::find_or<int>(*config,"audio","noise_suppress",-30),
        toml::find_or<int>(*config,"audio","jitter_buf_size",12)
    ))
        return false;
    auto conn = connect.lock();
    on_record_conn = audio_stream.reg_on_record_callback(sigc::mem_fun(*this,&VoiceStream::on_record_data));
    on_recv_connect = conn->on_recv_datagram.connect([this](const uint8_t* buf,size_t size){
        std::span<uint8_t> span((uint8_t*)buf,size);
#if DEBUG_LOG 
        LOG(DEBUG) << "voice recv " << size << " bytes";
#endif
        audio_stream.on_receive_data_def(span);
    });
    is_audio_stream_init = true;
    return true;
}

void VoiceStream::on_record_data(const std::span<uint8_t>& data, uint16_t framesPerBuffer)
{
    if(!is_ready || data.empty())
        return;
    auto conn = connect.lock();
    if(!conn)
        return;
    conn->write_datagram(data);
    auto success = conn->flush_datagram();
    assert(success);
    #if DEBUG_LOG 
        if(success)
            LOG(DEBUG) << "voice send " << data.size() << " bytes";
    #endif
}

void VoiceStream::close_audio_stream()
{
    if(on_record_conn.connected())
        on_record_conn.disconnect();
    if(is_audio_stream_init)
        audio_stream.close();
    if(on_recv_connect.connected())
        on_recv_connect.disconnect();
    is_audio_stream_init = false;
}

VoiceStream::~VoiceStream()
{
    close_audio_stream();
}