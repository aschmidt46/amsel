#include "mapper.h"
#include <assert.h>
#include <iostream>
#include <algorithm>
#include <format>

Mapper::Mapper(NESFile *cartridge, Cpu* cpu, Ppu* ppu)
{
    memoryMap = new uint8_t*[ADDRSPACE];
    this->ppu = ppu;
    this->cpu = cpu;
    this->cart = cartridge;

    assert(cart->header.getMapper()==0);

    // zu groß, aber egal?
    ppuMap = new uint8_t*[ADDRSPACE];
    for(unsigned int i = 0; i < ADDRSPACE; i++){
        memoryMap[i] = nullptr;
        ppuMap[i] = nullptr;
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
    PPUCTRL = &ppu->PPUCTRL;
    PPUMASK = &ppu->PPUMASK;
    PPUSTATUS = &ppu->PPUSTATUS;
    OAMADDR = &ppu->OAMADDR;
    OAMDATA = &ppu->OAMDATA;
    PPUSCROLL = &ppu->PPUSCROLL;
    PPUADDR = &ppu->PPUADDR;
    PPUDATA = &ppu->PPUDATA;
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
    //CPU
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

    // PPU
    // Größe in x*8KiB, 0 bedeutet CHR Ram
    assert(cartridge->header.CHRROMSize == 0 || cartridge->header.CHRROMSize == 1);

    for(int i = 0; i < 0x2000; i++){
        ppuMap[i] = cartridge->chrRom + i;
    }
    //Interner Ram
    index = 0x2000;
    for(int i = 0; i < 0x0800; i++){
        ppuMap[index + i] = ppu->internalMemory + i;
    }
    index = 0x2800;
    //Mirror
    for(int i = 0; i < 0x0800; i++){
        ppuMap[index + i] = ppu->internalMemory + i;
    }

    // Intern Mirror
    index = 0x3000;
    for(int i = 0; i < 0x0800; i++){
        ppuMap[index + i] = ppu->internalMemory + i;
    }
    index = 0x3800;
    //Mirror
    for(int i = 0; i < 0x0800 - 0x20; i++){
        ppuMap[index + i] = ppu->internalMemory + i;
    }

    // Palletten
    for(index = 0x3F00; index < 0x4000; index += 0x0020){
        for(int i = 0; i < 0x20; i++){
            ppuMap[index + i] = ppu->palletteIndexes + i;
        }
    }

    std::cout << "Mapper geladen." << std::endl;
}

uint8_t Mapper::read(uint8_t *address)
{
    [[unlikely]] if(memoryMap[(uintptr_t)address]==nullptr){
        std::cout << "Ungemapter Speicherzugriff, gebe 0 zurück" << std::endl;
        return 0;
    }
    auto val = *memoryMap[(uintptr_t)address];
    // PPU-Callback
    if((uintptr_t)address >= 0x2000 && (uintptr_t)address < 0x4000){
        ppu->readRegister(memoryMap[(uintptr_t)address]);
    }
    return val;
}

void Mapper::write(uint8_t *address, uint8_t value)
{
    [[unlikely]] if(memoryMap[(uintptr_t)address] == nullptr){
        std::cout << "Ungemapter Speicher-Schreib-Zugriff, tue nichts" << std::endl;
        return;
    }
    *memoryMap[(uintptr_t)address] = value;
    // PPU-Callback
    if((uintptr_t)address >= 0x2000 && (uintptr_t)address < 0x4000){
        ppu->wroteRegister(memoryMap[(uintptr_t)address]);
    }
    // OAMDMA write
    else [[unlikely]] if((uintptr_t)address == 0x4014){
        uint8_t cpuPage = read(address);
        uint16_t cpuAddress = ((uint16_t) cpuPage) << 8;
        for(int i=0; i < 256; i++){
            uint8_t val = read((uint8_t*)(uintptr_t)cpuAddress + i);
            ppu->OAM[i] = val;
        }
        cpu->remainingCycles += 513; // Oder 514???
    }
}

uint8_t Mapper::readVRAM(uint8_t *address)
{
    [[unlikely]] if(ppuMap[(uintptr_t)address]==nullptr){
        std::cout << "Ungemapter VRAM-Zugriff, gebe 0 zurück" << std::endl;
        return 0;
    }
    return *ppuMap[(uintptr_t)address];
}

std::string mhex(uintptr_t input){
    std::string str = std::format("{:x}", input);
    std::transform(str.begin(), str.end(), str.begin(), ::toupper);
    return str;
}

void Mapper::writeVRAM(uint8_t *address, uint8_t value)
{
    [[unlikely]] if(ppuMap[(uintptr_t)address] == nullptr){
        std::cout << "Ungemapter VRAM-Schreib-Zugriff bei " << mhex((uintptr_t)address) << ", tue nichts" << std::endl;
        throw;
        return;
    }
    *ppuMap[(uintptr_t)address] = value;
}

void Mapper::pullNMI()
{
    cpu->pullNMI();
}
