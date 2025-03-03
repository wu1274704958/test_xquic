#pragma once

#include <portaudio.h>

//initialize portaudio

class PortAudioGlobal
{
    public:
    PortAudioGlobal();
    ~PortAudioGlobal();
    private:
    static void EasyLogInitForPortAudio();
    static void PaLogCallback(const char *log);
};
