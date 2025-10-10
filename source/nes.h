#pragma once
#include "6502.h"
#include "ppu.h"
#include "screen.h"
#include "mapper.h"
#include "nes_file.h"


class Cpu;
class Ppu;
class Mapper;
struct NESFile;
class Screen;

class NES{
    Cpu* cpu;
    Ppu* ppu;
    Mapper* mapper;
    NESFile* Slot;
    Screen* tv;
    
    public:
    NES(Screen* screen);
    void load(const char* path);
    void eject();
    void reset();
    void nextFrame();
};
