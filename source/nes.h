#pragma once
#include "6502.h"
#include "ppu.h"
#include "screen.h"
#include "mapper.h"
#include "nes_file.h"
#include <chrono>


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
    // Controller* controller;

    std::chrono::time_point<std::chrono::high_resolution_clock> t1 = std::chrono::high_resolution_clock::now();
    
    public:
    NES(Screen* screen);
    void load(const char* path);
    void eject();
    void reset();
    void nextFrame();
};
