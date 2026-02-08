#pragma once
#include <cstdint>
#include "nes_file.h"
#include "6502.h"
#include "ppu.h"
#include <limits>
#include "controller.h"
#include "apu/apu.h"
#include "abstract_mapper.h"
#include "mappers/mapper0.h"
#include "mappers/mapper2.h"
#include "mappers/mapper3.h"

constexpr const size_t ADDRSPACE = 1 + (size_t) std::numeric_limits<uint16_t>::max();

enum Mirror{
    MIRROR_VERTICAL,
    MIRROR_HORIZONTAL
};

class Cpu;
class Ppu;
class Apu;
class Controller;
class AbstractMapper;
class Mapper : virtual public std::enable_shared_from_this<Mapper>{
    public:
    uint8_t** memoryMap = nullptr;    // CPU-Adressraum
    uint8_t** ppuMap = nullptr;       // PPU-Adressraum

    
    Cpu* cpu = nullptr;
    Ppu* ppu = nullptr;
    Apu* apu = nullptr;
    Controller* controller1 = nullptr;
    NESFile* cart = nullptr;
    uint8_t* prgRam = nullptr;
    //muss noch implementiert werden
    uint8_t* io = nullptr;

    
    // Adressen der öffentlichen PPU-Register
    uint8_t* PPUCTRL = nullptr;
    uint8_t* PPUMASK = nullptr;
    uint8_t* PPUSTATUS = nullptr;
    uint8_t* OAMADDR = nullptr;
    uint8_t* OAMDATA = nullptr;
    uint8_t* PPUSCROLL = nullptr;
    uint8_t* PPUADDR = nullptr;
    uint8_t* PPUDATA = nullptr;
    
    uint8_t controller[2];
    uint8_t controller_state[2];



    bool chrRAM = false;

    Mapper(Cpu* cpu, Ppu* ppu, Apu* apu);
    ~Mapper(){
        delete[] memoryMap;
        delete[] ppuMap;
        delete[] io;
        delete[] prgRam;
    };

    void changeCart(NESFile* cartridge);
    void connectController(Controller* controller);
    
    Mirror mirror = MIRROR_HORIZONTAL; //Muss noch separat initialisiert werden

    AbstractMapper* mImpl = nullptr;

    uint8_t read(uint8_t* address);
    void write(uint8_t* address, uint8_t value);
    uint8_t readVRAM(uint8_t* address);
    void writeVRAM(uint8_t* address, uint8_t value);
    void pullNMI();
    void pullIRQ();
};
