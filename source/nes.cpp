#include "nes.h"

using std::chrono::high_resolution_clock;
using std::chrono::duration_cast;
using std::chrono::duration;
using std::chrono::milliseconds;
using std::chrono::microseconds;
using std::chrono::nanoseconds;
using std::chrono::seconds;

NES::NES(Screen* screen, Controller* c){
    cpu = new Cpu();
    ppu = new Ppu();
    apu = new Apu();
    tv = screen;
    controller = c;
}

void NES::load(const char *path)
{
    if(loaded)
        eject();
    Slot = new NESFile(path);
    mapper = new Mapper(Slot, cpu, ppu, apu, controller);
    cpu->init(0xC000, mapper);
    ppu->init(mapper, tv);
    loaded = true;
}

void NES::eject()
{
    if(!loaded) return;
    delete Slot;
    delete mapper;
    loaded = false;
}

void NES::reset()
{
    if(loaded)
    cpu->RESET();
}

// NTSC
constexpr const double nsPerClock = 558.73007359033799258341700316185;

void NES::nextFrame()
{
    t1 = high_resolution_clock::now();
    while(!ppu->frameReady){
        auto t2 = high_resolution_clock::now();
        while(duration_cast<nanoseconds>(t2-t1).count() < nsPerClock) {
            t2 = high_resolution_clock::now();
        }
        
        ppu->clock();
        ppu->clock();
        ppu->clock();
        cpu->clockCPU();
        controller->clock();

        t1 = high_resolution_clock::now();
    }
    ppu->frameReady = false;
    tv->present();
}

bool NES::clock()
{
    if(!loaded) return false;
    ppu->clock();
    apu->clock();
    numClocks++;
    if(numClocks%3==0){
        cpu->clockCPU();
        controller->clock();
        numClocks = 0;
    }
    if(ppu->frameReady){
        frameReady = true;
        ppu->frameReady = false;
    }
    bool audioSampleReady = false;
    audioTime += audioTimePerNESClock;
    if(audioTime >= audioTimePerSystemSample){
        audioTime -= audioTimePerSystemSample;
        audioSample = apu->getSample();
        audioSampleReady = true;
    }

    return audioSampleReady;
}
