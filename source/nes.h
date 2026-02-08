#pragma once
#include "6502.h"
#include "ppu.h"
#include "screen.h"
#include "mapper.h"
#include "nes_file.h"
#include <chrono>
#include "controller.h"
#include "apu/apu.h"
#include <string>
#include "global.h"

class Cpu;
class Ppu;
class Apu;
class Mapper;
struct NESFile;
class Screen;
class Controller;

class NES{
    Cpu* cpu;
    Apu* apu;
    std::shared_ptr<Mapper> mapper = nullptr;
    NESFile* Slot;
    Screen* tv;
    Controller* controller;
    
    std::chrono::time_point<std::chrono::high_resolution_clock> t1 = std::chrono::high_resolution_clock::now();
    
    const double sampleRate = 20000;
    const double audioTimePerSystemSample = 1.0 / sampleRate;
    const double audioTimePerNESClock = 1.0 / 5369318.0; // ppu clock
    double audioTime = 0.0;
    bool loaded = false;
    
    
    public:
    bool ejectNextClock = false, loadNextClock = false, resetNextClock = false;
    std::string fileName;
    double audioSample;
    float volume = 1.0f;
    Ppu* ppu;
    bool frameReady = false;
    bool sound = true;
    NES(Screen* screen, Controller* c);
    void load(const char* path);
    void eject();
    void reset();
    void nextFrame();
    bool clock();
    int numClocks = 0;


    // Debugging
    bool produceDisassembly = false;
    int assemblyLines = 10;
    bool halt = false;
    int allowedClocks = 0;
    std::string getCurrentDisassembly();

    private:
    std::string ASM = "";
    std::mutex debugM;
};
