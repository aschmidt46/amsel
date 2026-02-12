#include "nes.h"

using std::chrono::high_resolution_clock;
using std::chrono::duration_cast;
using std::chrono::duration;
using std::chrono::milliseconds;
using std::chrono::microseconds;
using std::chrono::nanoseconds;
using std::chrono::seconds;

NES::NES(Screen* screen, Controller* c){
    tv = screen;
    cpu = new Cpu();
    ppu = new Ppu();
    apu = new Apu();
    controller = c;
    mapper = std::make_shared<Mapper>(cpu, ppu, apu);
    mapper->connectController(controller); 
}

NES::~NES()
{
    delete apu;
    delete ppu;
    delete cpu;
}

void NES::load(const char *path)
{
    std::lock_guard<std::mutex> lock(cvm);
    auto Slot = std::make_shared<NESFile>(path);
    if(mapper->changeCart(Slot)){
        apu->reset(mapper);
        ppu->init(mapper);
        cpu->init(0xC000, mapper);
        loaded = true;
        changeTitle = true;
    }
    else{
        unimplementedMapper = Slot->header.getMapper();
    }
    loadNextClock = false;
}

void NES::eject()
{
    if(!loaded) return;
    fileName = "";
    loaded = false;
    ejectNextClock = false;
    changeTitle = true;
}

void NES::reset()
{
    if(loaded)
    cpu->RESET();
    mapper->reset();
    resetNextClock = false;
}

bool NES::clock()
{
    if(loadNextClock) load(fileName.c_str());
    if(ejectNextClock) eject();
    if(resetNextClock) reset();
    if(!loaded) return false;
    
    //Debug
    if(watchBreakpoints && allowedClocks == 0){
        if(std::find(breakpoints.begin(), breakpoints.end(), cpu->PC) != breakpoints.end()){
            halt = true;
        }
        if(std::find(breakpointsOP.begin(), breakpointsOP.end(), cpu->opcodes[cpu->read((uint8_t*)(uintptr_t)cpu->PC)].name) != breakpointsOP.end()){
            halt = true;
        }
    }
    if(halt && allowedClocks==0) return false;

    ppu->clock();
    numClocks++;
    if(numClocks%3==0){
        if(produceDisassembly)
            std::lock_guard<std::mutex> lock(debugM);
        bool cpuAdvanced = cpu->clockCPU();
        apu->clock();
        controller->clock();
        numClocks = 0;

        // Debug
        if(cpuAdvanced && produceDisassembly){
            if(allowedClocks > 0)
                allowedClocks--;
        }
    }
    if(ppu->frameReady){
        frameReady = true;
        ppu->frameReady = false;
    }
    bool audioSampleReady = false;
    audioTime += audioTimePerNESClock;
    if(audioTime >= audioTimePerSystemSample){
        audioTime -= audioTimePerSystemSample;
        audioSample = apu->getSample(sound) * volume;
        audioSampleReady = true;
    }

    return audioSampleReady;
}









// Debug Funktionen

std::pair<std::string, std::vector<int>> NES::getCurrentDisassembly()
{
    std::lock_guard<std::mutex> lock(debugM);
    if(!loaded)
        return {"", {}};
    return cpu->getNextNInstructions(10);
}

std::pair<std::string, std::vector<int>> NES::getOldDisassembly()
{
    std::lock_guard<std::mutex> lock(debugM);
    if(!loaded)
        return {"", {}};
    return cpu->getPrev10Instructions();
}

std::vector<uint16_t> NES::addBreakpoint(uint16_t bp)
{
    if(!watchBreakpoints){
        watchBreakpoints = true;
    }

    bool exists = false;

    for(const auto &e : breakpoints){
        if(e==bp)
            exists = true;
    }

    if(!exists){
        breakpoints.push_back(bp);
    }

    return breakpoints;
}

std::vector<uint16_t> NES::removeBreakpoint(uint16_t bp)
{
    breakpoints.erase(std::remove_if(breakpoints.begin(), breakpoints.end(), 
                       [&](uint16_t i) { return i == bp; }), breakpoints.end());


    if(watchBreakpoints && breakpointsOP.size() == 0 && breakpoints.size() == 0){
        watchBreakpoints = false;
    }
    return breakpoints;
}

std::vector<std::string> NES::addBreakpointOP(std::string bp)
{
    
    if(!watchBreakpoints){
        watchBreakpoints = true;
    }

    bool exists = false;

    for(const auto &e : breakpointsOP){
        if(e==bp)
            exists = true;
    }

    if(!exists){
        breakpointsOP.push_back(bp);
    }

    return breakpointsOP;
}

std::vector<std::string> NES::removeBreakpointOP(std::string bp)
{
    breakpointsOP.erase(std::remove_if(breakpointsOP.begin(), breakpointsOP.end(), 
                       [&](std::string i) { return i == bp; }), breakpointsOP.end());


    if(watchBreakpoints && breakpointsOP.size() == 0 && breakpoints.size() == 0){
        watchBreakpoints = false;
    }
    return breakpointsOP;
}

std::string NES::getText(uint16_t addr)
{
    if(!loaded){
        return "";
    }
    std::string res = "";
    int max = 2000;
    int i = 0;
    while(addr < 0xFFFF && i < max){
        char c = mapper->read((uint8_t*)(uintptr_t)addr);
        if(c == '\0')
            break;
        res += c;
        addr++;
        i++;
    }
    return res;
}
