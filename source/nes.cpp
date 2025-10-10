#include "nes.h"

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
}

void NES::reset()
{
}

void NES::nextFrame()
{
    while(!ppu->frameReady){
        ppu->clock();
        ppu->clock();
        ppu->clock();
        cpu->clockCPU();
    }
    ppu->frameReady = false;
}
