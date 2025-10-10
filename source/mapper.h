#pragma once
#include <cstdint>
#include "nes_file.h"
#include "6502.h"
#include "ppu.h"
#include <limits>

constexpr const size_t ADDRSPACE = 1 + (size_t) std::numeric_limits<uint16_t>::max();

class Cpu;
class Ppu;
class Mapper{
    uint8_t** memoryMap;    // CPU-Adressraum
    uint8_t** ppuMap;       // PPU-Adressraum

    Cpu* cpu;
    Ppu* ppu;
    NESFile* cart;
    uint8_t* prgRam;
    //muss noch implementiert werden
    uint8_t* io;

    // Adressen der öffentlichen PPU-Register
    uint8_t* PPUCTRL;
    uint8_t* PPUMASK;
    uint8_t* PPUSTATUS;
    uint8_t* OAMADDR;
    uint8_t* OAMDATA;
    uint8_t* PPUSCROLL;
    uint8_t* PPUADDR;
    uint8_t* PPUDATA;

    public:
    Mapper(NESFile* cartridge, Cpu* cpu, Ppu* ppu);
    ~Mapper(){
        delete[] memoryMap;
        delete[] ppuMap;
        delete[] io;
        delete[] prgRam;
    };

    uint8_t read(uint8_t* address);
    void write(uint8_t* address, uint8_t value);
    uint8_t readVRAM(uint8_t* address);
    void writeVRAM(uint8_t* address, uint8_t value);
    void pullNMI();
};
