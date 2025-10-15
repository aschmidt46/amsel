#pragma once
#include "nes.h"
#include "RtAudio.h"

class NES;
class AudioSystem{
    const double sampleRate = 20000;
    const double audioTimePerSystemSample = 1.0 / sampleRate;
    const double audioTimePerNESClock = 1.0 / 5369318.0; // ppu clock
    double audioTime = 0.0;
    
    NES* nes;
    
    RtAudio dac;
    
    public:
    bool close = false;
    AudioSystem(NES* console);
    ~AudioSystem();
    void start();
};
