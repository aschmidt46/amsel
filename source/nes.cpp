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
    std::lock_guard<std::mutex> lock(cvm);
    if(loaded)
        eject();
    Slot = new NESFile(path);
    if(mapper==nullptr){
        mapper = std::shared_ptr<Mapper>(std::make_shared<Mapper>(cpu, ppu, apu));
    }
    assert(mapper.use_count() > 0);
    mapper->changeCart(Slot);
    mapper->connectController(controller); 
    apu->reset(mapper);
    ppu->init(mapper);
    cpu->init(0xC000, mapper);
    loaded = true;
    loadNextClock = false;
}

void NES::eject()
{
    if(!loaded) return;
    delete Slot;
    loaded = false;
    ejectNextClock = false;
}

void NES::reset()
{
    if(loaded)
    cpu->RESET();
    resetNextClock = false;
}

// NTSC
constexpr const double nsPerClock = 558.73007359033799258341700316185;

// Veraltet
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
    if(loadNextClock) load(fileName.c_str());
    if(ejectNextClock) eject();
    if(resetNextClock) reset();
    if(!loaded) return false;
    ppu->clock();
    numClocks++;
    if(numClocks%3==0){
        cpu->clockCPU();
        apu->clock();
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
        audioSample = apu->getSample(sound) * volume;
        audioSampleReady = true;
    }

    return audioSampleReady;
}
