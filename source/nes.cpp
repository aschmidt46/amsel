#include "nes.h"

using std::chrono::high_resolution_clock;
using std::chrono::duration_cast;
using std::chrono::duration;
using std::chrono::milliseconds;
using std::chrono::microseconds;
using std::chrono::nanoseconds;
using std::chrono::seconds;

NES::NES(Screen* screen){
    cpu = new Cpu();
    ppu = new Ppu();
    tv = screen;
}

void NES::load(const char *path)
{
    Slot = new NESFile(path);
    mapper = new Mapper(Slot, cpu, ppu);
    cpu->init(0xC000, mapper);
    ppu->init(mapper, tv);
}

void NES::eject()
{
    delete Slot;
    delete mapper;
}

void NES::reset()
{
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
        cpu->clockCPU();
        ppu->clock();
        ppu->clock();
        ppu->clock();
        t1 = high_resolution_clock::now();
    }
    ppu->frameReady = false;
    tv->copyBufferToScreen(ppu->pixelBuffer);
    tv->present();
}
