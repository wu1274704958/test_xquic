#pragma once

#include <portaudio.h>

//initialize portaudio

class PortAudioGlobal
{
    public:
    PortAudioGlobal(const char* log_file);
    ~PortAudioGlobal();
    private:
    static void EasyLogInitForPortAudio(const char* log_file);
    static void PaLogCallback(const char *log);
};
