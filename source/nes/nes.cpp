#include "nes.h"

using std::chrono::high_resolution_clock;
using std::chrono::duration_cast;
using std::chrono::duration;
using std::chrono::milliseconds;
using std::chrono::microseconds;
using std::chrono::nanoseconds;
using std::chrono::seconds;

NES::NES(){
    cpu = new Cpu();
    ppu = new Ppu();
    apu = new Apu();
    controller1 = new Controller(false);
    controller2 = new Controller(true);
    mapper = std::make_shared<Mapper>(cpu, ppu, apu);
    mapper->connectController(controller1, controller2); 
}

NES::~NES()
{
    delete apu;
    delete ppu;
    delete cpu;
    delete controller1;
    delete controller2;
}

void NES::load(const char *path)
{
    // std::lock_guard<std::mutex> lock(framebufferM);
    auto Slot = std::make_shared<NESFile>(path);
    if(mapper->changeCart(Slot)){
        apu->reset(mapper);
        ppu->init(mapper);
        cpu->init(0xC000, mapper);
        changeTitle = true;
        loaded = true;
    }
    else{
        MessageStruct m = {
            .type=MT_ERROR,
            .title="Error",
            .content="Dieses Spiel (iNES-Mapper "+std::to_string(Slot->header.getMapper())+") wird nicht unterstützt, weil der entsprechende Mapper nicht implementiert ist."
        };
        messageQueue.enqueue(m);
    }
}

void NES::eject()
{
    // if(!loaded) return;
    // fileName = "";
    // loaded = false;
    // ejectNextClock = false;
    // changeTitle = true;
}

void NES::reset()
{
    // if(loaded)
    // cpu->RESET();
    // mapper->reset();
    // resetNextClock = false;
}

void NES::clock()
{
    // if(loadNextClock) load(fileName.c_str());
    // if(ejectNextClock) eject();
    // if(resetNextClock) reset();
    if(!loaded) return;
    
    //Debug
    if(watchBreakpoints && allowedClocks == 0){
        if(std::find(breakpoints.begin(), breakpoints.end(), cpu->PC) != breakpoints.end()){
            halt = true;
        }
        if(std::find(breakpointsOP.begin(), breakpointsOP.end(), cpu->opcodes[cpu->read((uint8_t*)(uintptr_t)cpu->PC)].name) != breakpointsOP.end()){
            halt = true;
        }
    }
    if(halt && allowedClocks==0) return;

    ppu->clock();
    numClocks++;
    if(numClocks%3==0){
        if(produceDisassembly)
            std::lock_guard<std::mutex> lock(debugM);
        bool cpuAdvanced = cpu->clockCPU();
        apu->clock();
        controller1->clock();
        controller2->clock();
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
        audioSample = apu->getSample(true);
        audioSampleReady = true;
    }

    this->newAudioSample = audioSampleReady;
}

float *NES::accessFramebuffer()
{
    return ppu->backBuffer;
}

bool NES::frameIsReady()
{
    auto tmp = this->frameReady;
    if(tmp) this->frameReady = false;
    return tmp;
}

bool NES::hasAudioSample()
{
    auto tmp = this->newAudioSample;
    if(tmp) this->newAudioSample = false;
    return tmp;
}

bool NES::shouldChangeTitle()
{
    auto tmp = this->changeTitle;
    if(tmp) this->changeTitle = false;
    return tmp;
}

std::string NES::getTitle()
{
    return fileName;
}

double NES::getSample()
{
    return audioSample;
}

bool NES::isLoaded()
{
    return loaded;
}

float NES::getX()
{
    return this->x;
}

float NES::getY()
{
    return this->y;
}

void NES::setController1Key(bool gamepad, int key, int action)
{
    controller1->setKey(gamepad, key, action);
}

void NES::setController2Key(bool gamepad, int key, int action)
{
    controller2->setKey(gamepad, key, action);
}

// Debug Funktionen

std::pair<std::string, std::vector<int>> NES::getCurrentDisassembly()
{
    std::lock_guard<std::mutex> lock(debugM);
    // if(!loaded)
    //     return {"", {}};
    return cpu->getNextNInstructions(10);
}

std::pair<std::string, std::vector<int>> NES::getOldDisassembly()
{
    std::lock_guard<std::mutex> lock(debugM);
    // if(!loaded)
    //     return {"", {}};
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
    // if(!loaded){
    //     return "";
    // }
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

std::string NES::getOpcodeName(size_t index)
{
    return this->cpu->opcodes[index].name;
}

uint8_t NES::readCpuBus(uint16_t addr)
{
    return this->cpu->read((uint8_t*)(uintptr_t)addr);
}

uint16_t NES::readRegister(CpuReg reg)
{
    switch(reg){
        case RegP: return this->cpu->P;
        case RegPC: return this->cpu->PC;
        case RegSP: return this->cpu->SP;
        case RegA: return this->cpu->A;
        case RegX: return this->cpu->X;
        case RegY: return this->cpu->Y;
    }
    return 0;
}
