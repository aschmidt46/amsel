#pragma once
#include "6502.h"
#include "ppu.h"
#include "mapper.h"
#include "nes_file.h"
#include <chrono>
#include "controller.h"
#include "apu/apu.h"
#include <string>
#include "framework/global.h"

class Cpu;
class Ppu;
class Apu;
class Mapper;
struct NESFile;
class Controller;

class NES{
    public:
    Cpu* cpu;
    private:
    Apu* apu;
    Controller* controller1;
    Controller* controller2;
    std::shared_ptr<Mapper> mapper = nullptr;
    
    std::chrono::time_point<std::chrono::high_resolution_clock> t1 = std::chrono::high_resolution_clock::now();
    
    const double sampleRate = 20000;
    const double audioTimePerSystemSample = 1.0 / sampleRate;
    const double audioTimePerNESClock = 1.0 / 5369318.0; // ppu clock
    double audioTime = 0.0;
    
    
    public:
    bool loaded = false;
    bool ejectNextClock = false, loadNextClock = false, resetNextClock = false;
    bool changeTitle = false;
    std::string fileName;
    double audioSample;
    Ppu* ppu;
    bool frameReady = false;
    bool sound = true;
    NES(Screen* screen);
    ~NES();
    void load(const char* path);
    void eject();
    void reset();
    bool clock();
    // Für CPU-Sync
    int numClocks = 0;

    void setController1Key(bool gamepad, int key, int action);
    void setController2Key(bool gamepad, int key, int action);










    // Debugging
    bool produceDisassembly = false;
    int assemblyLines = 10;
    bool halt = false;
    int allowedClocks = 0;
    std::pair<std::string, std::vector<int>> getCurrentDisassembly();
    std::pair<std::string, std::vector<int>> getOldDisassembly();
    std::vector<uint16_t> addBreakpoint(uint16_t bp);
    std::vector<uint16_t> removeBreakpoint(uint16_t bp);
    std::vector<std::string> addBreakpointOP(std::string bp);
    std::vector<std::string> removeBreakpointOP(std::string bp);
    std::string getText(uint16_t addr);

    private:
    std::mutex debugM;

    bool watchBreakpoints = false;
    std::vector<uint16_t> breakpoints;
    std::vector<std::string> breakpointsOP;
};
