#include "PortAudioGlobal.h"
#include "easylogging++.h"
#include <mqas/comm/string.h>

extern "C" {
typedef void (*PaUtilLogCallback ) (const char *log);
extern void PaUtil_SetDebugPrintFunction(PaUtilLogCallback  cb);
}
        
//initialize easylogging
void PortAudioGlobal::EasyLogInitForPortAudio()
{
    const auto logger = el::Loggers::getLogger("portaudio");
	el::Configurations c;
	c.setFromBase(el::Loggers::getLogger("default")->configurations());
	auto fmt = c.get(el::Level::Global, el::ConfigurationType::Format)->value();
	bool erase_succ = mqas::comm::erase_substr(fmt, "[%level]");
	if (!erase_succ) erase_succ = mqas::comm::erase_substr(fmt, "[%levshort]");
	if (erase_succ)
		c.set(el::Level::Global, el::ConfigurationType::Format, fmt);
	el::Loggers::reconfigureLogger(logger, c);
}

void PortAudioGlobal::PaLogCallback(const char *log)
{
    CLOG(INFO,"portaudio") << log;
}

//initialize portaudio
PortAudioGlobal::PortAudioGlobal()
{
    EasyLogInitForPortAudio();
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