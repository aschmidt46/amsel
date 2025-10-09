#include "mapper.h"
#include <limits>
#include <assert.h>
#include <iostream>

Mapper::Mapper(NESFile *cartridge, Cpu* cpu, Ppu* ppu)
{
    memoryMap = new uint8_t*[std::numeric_limits<uint16_t>::max()];
    for(unsigned int i = 0; i < std::numeric_limits<uint16_t>::max(); i++){
        memoryMap[i] = nullptr;
    }

    int index = 0;
    // Interner Ram + 3 Mirrors vom internen Ram
    for(int j = 0; j < 4; j++){
        for(int i = 0; i < 0x0800; i++){
            memoryMap[index + i] = cpu->internalMemory + i;
        }
        index += 0x0800;
    }
    assert(index == 0x2000);
    // PPU Register
    for(int i = index; i < 0x4000; i+=0x8){
        memoryMap[i]   = &ppu->PPUCTRL;
        memoryMap[i+1] = &ppu->PPUMASK;
        memoryMap[i+2] = &ppu->PPUSTATUS;
        memoryMap[i+3] = &ppu->OAMADDR;
        memoryMap[i+4] = &ppu->OAMDATA;
        memoryMap[i+5] = &ppu->PPUSCROLL;
        memoryMap[i+6] = &ppu->PPUADDR;
        memoryMap[i+7] = &ppu->PPUDATA;
    }
    index = 0x4000;
    
    // IO Register (nicht implementiert)
    io = new uint8_t[0x18];
    for(int i = 0; i < 0x18; i++){
        memoryMap[i + index] = io + i;
    }
    index += 0x18;

    //APU and I/O functionality that is normally disabled.
    index += 0x8;

    //Cartridge
    //Mapper0
    assert(cartridge->header.PRGROMSize == 1 || cartridge->header.PRGROMSize == 2);

    //PRG-RAM 8KB
    index = 0x6000;
    prgRam = new uint8_t[0x2000];
    for(int i = 0; i < 0x2000; i++){
        memoryMap[index + i] = prgRam + i;
    }
    
    // 16-32KB ROM
    index = 0x8000;
    if(cartridge->header.PRGROMSize == 1){
        for(int i = 0; i < 0x4000; i++){
            memoryMap[index + i] = cartridge->prgRom + i;
        }
        //Mirror
        index += 0x4000;
        for(int i = 0; i < 0x4000; i++){
            memoryMap[index + i] = cartridge->prgRom + i;
        }
    }
    else{
        //32KB
        for(int i = 0; i < 0x8000; i++){
            memoryMap[index + i] = cartridge->prgRom + i;
        }
    }
}

uint8_t Mapper::read(uint8_t *address)
{
    if(memoryMap[(uintptr_t)address]==nullptr){
        std::cout << "Ungemapter Speicherzugriff, gebe 0 zurück" << std::endl;
        return 0;
    }
    return *memoryMap[(uintptr_t)address];
}

void Mapper::write(uint8_t *address, uint8_t value)
{
    if(memoryMap[(uintptr_t)address] == nullptr){
        std::cout << "Ungemapter Speicherzugriff, tue nichts" << std::endl;
        return;
    }
    *memoryMap[(uintptr_t)address] = value;
}
