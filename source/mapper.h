#pragma once
#include <cstdint>
#include "nes_file.h"
#include "6502.h"
#include "ppu.h"

class Cpu;
class Ppu;
class Mapper{
    uint8_t** memoryMap;
    Cpu* cpu;
    Ppu* ppu;
    NESFile* cart;
    //muss noch implementiert werden
    uint8_t* io;
    uint8_t* prgRam;
    public:
    Mapper(NESFile* cartridge, Cpu* cpu, Ppu* ppu);
    ~Mapper(){
        delete[] memoryMap;
        delete[] io;
        delete[] prgRam;
    };

    uint8_t read(uint8_t* address);
    void write(uint8_t* address, uint8_t value);
};
