#include "PortAudioGlobal.h"
#include "easylogging++.h"
#include <mqas/comm/string.h>

extern "C" {
typedef void (*PaUtilLogCallback ) (const char *log);
extern void PaUtil_SetDebugPrintFunction(PaUtilLogCallback  cb);
}
        
//initialize easylogging
void PortAudioGlobal::EasyLogInitForPortAudio(const char* log_file)
{
    el::Configurations c;
    c.setToDefault();
    c.parseFromFile(log_file);

    const auto portaudio_logger = el::Loggers::getLogger("portaudio");
    const auto audio_logger = el::Loggers::getLogger("audio");

	el::Loggers::reconfigureLogger(portaudio_logger, c);
    el::Loggers::reconfigureLogger(audio_logger, c);
}

void PortAudioGlobal::PaLogCallback(const char *log)
{
    CLOG(INFO,"portaudio") << log;
}

//initialize portaudio
PortAudioGlobal::PortAudioGlobal(const char* log_file)
{
    EasyLogInitForPortAudio(log_file);
    PaUtil_SetDebugPrintFunction(PaLogCallback);
    PaError pa_err = Pa_Initialize();

    if (pa_err != paNoError) {
        throw new std::runtime_error("PortAudio initialization failed: " + std::string(Pa_GetErrorText(pa_err)));
    }
}   

PortAudioGlobal::~PortAudioGlobal()
{
    Pa_Terminate();
}