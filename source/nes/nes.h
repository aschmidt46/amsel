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

enum CpuReg : unsigned int{
    RegP = 0,
    RegPC = 1,
    RegSP = 2,
    RegA = 3,
    RegX = 4,
    RegY = 5,
};

class NES{
    private:
    std::shared_ptr<Ppu> ppu;
    std::shared_ptr<Cpu> cpu;
    std::shared_ptr<Apu> apu;
    std::shared_ptr<Controller> controller1;
    std::shared_ptr<Controller> controller2;
    std::shared_ptr<Mapper> mapper = nullptr;
    
    std::chrono::time_point<std::chrono::high_resolution_clock> t1 = std::chrono::high_resolution_clock::now();
    
    const double sampleRate = 20000;
    const double audioTimePerSystemSample = 1.0 / sampleRate;
    const double audioTimePerNESClock = 1.0 / 5369318.0; // ppu clock
    double audioTime = 0.0;
    const float x = 256.0f;
    const float y = 240.0f;
    
    bool frameReady = false;
    bool changeTitle = false;
    std::string fileName;
    double audioSample;
    bool loaded = false;
    // Für CPU-Sync
    int numClocks = 0;
    bool newAudioSample = false;
    
    public:
    NES();
    ~NES();
    void load(std::vector<uint8_t> &rom);
    void load(const char* path);
    void eject();
    void reset();
    void clock();
    float* accessFramebuffer();
    bool frameIsReady();
    bool hasAudioSample();
    bool shouldChangeTitle();
    std::string getTitle();
    double getSample();
    bool isLoaded();
    float getX();
    float getY();

    bool canSave();
    std::vector<uint8_t> getSaveData();


    void setController1Key(bool gamepad, int key, int action);
    void setController2Key(bool gamepad, int key, int action);

    // template<class Archive>
    // void serialize(Archive & archive)
    // {
    //   archive( x, y, z ); // serialize things by passing them to the archive
    // }










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
    std::string getOpcodeName(size_t index);
    uint8_t readCpuBus(uint16_t addr);
    uint16_t readRegister(CpuReg reg);

    private:
    std::mutex debugM;

    bool watchBreakpoints = false;
    std::vector<uint16_t> breakpoints;
    std::vector<std::string> breakpointsOP;
};
