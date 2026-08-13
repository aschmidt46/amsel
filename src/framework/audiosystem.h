#pragma once
#include "nes/nes.h"
#include "RtAudio.h"

class NES;
class AudioSystem{
    const double sampleRate = 20000;
    
    RtAudio dac;
    
    public:
    bool close = false;
    AudioSystem();
    ~AudioSystem();
    void start();
};
