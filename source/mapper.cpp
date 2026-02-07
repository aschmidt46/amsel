#include "mapper.h"
#include <assert.h>
#include <iostream>
#include <algorithm>
#include <format>

std::string mhex(uintptr_t input){
    std::string str = std::format("{:x}", input);
    std::transform(str.begin(), str.end(), str.begin(), ::toupper);
    return str;
}


Mapper::Mapper(NESFile *cartridge, Cpu* cpu, Ppu* ppu, Apu* apu, Controller* c)
{
    memoryMap = new uint8_t*[ADDRSPACE];
    this->ppu = ppu;
    this->cpu = cpu;
    this->apu = apu;
    this->controller1 = c;

    controller1->init(this);

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
        memoryMap[i]   = (uint8_t*)&ppu->PPUCTRL;
        memoryMap[i+1] = (uint8_t*)&ppu->PPUMASK;
        memoryMap[i+2] = (uint8_t*)&ppu->PPUSTATUS;
        memoryMap[i+3] = (uint8_t*)&ppu->OAMADDR;
        memoryMap[i+4] = (uint8_t*)&ppu->OAMDATA;
        memoryMap[i+5] = (uint8_t*)&ppu->PPUSCROLL;
        memoryMap[i+6] = (uint8_t*)&ppu->PPUADDR;
        memoryMap[i+7] = (uint8_t*)&ppu->PPUDATA;
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
    //CPU
    //assert(cartridge->header.PRGROMSize == 1 || cartridge->header.PRGROMSize == 2);

    //PRG-RAM 8KB
    index = 0x6000;
    prgRam = new uint8_t[0x2000];
    for(int i = 0; i < 0x2000; i++){
        memoryMap[index + i] = prgRam + i;
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

    // Palletten gespiegelt bis 0x4000
    for(index = 0x3F00; index < 0x4000; index += 0x0020){
        for(int i = 0; i < 0x20; i++){
            auto pIndex = ppu->palletteIndexes;
            int j = i;
            // Innerhalb zeigen alle diese Register auf das gleiche innere Register (Backdrop Farbe)
            if (i == 0x0010) j = 0x0000;
	        if (i == 0x0014) j = 0x0004;
	        if (i == 0x0018) j = 0x0008;
	        if (i == 0x001C) j = 0x000C;
            
            ppuMap[index + i] = pIndex + j;
        }
    }

    changeCart(cartridge);
}

void Mapper::changeCart(NESFile *cartridge)
{
    this->cart = cartridge;
    int mappernum = cart->header.getMapper();

    // 16-32KB ROM
    int index = 0x8000;
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
    mirror = cart->header.flags6.getNametableArrangement() ? MIRROR_VERTICAL : MIRROR_HORIZONTAL;
    // Größe in x*8KiB, 0 bedeutet CHR Ram
    assert(cartridge->header.CHRROMSize == 0 || cartridge->header.CHRROMSize == 1);

    for(int i = 0; i < 0x2000; i++){
        ppuMap[i] = cartridge->chrRom + i;
    }
    //Interner Ram
    if(mirror == MIRROR_VERTICAL){
        std::cout << "Horizontale Ausrichtung" << std::endl;
        index = 0x2000;
        for(int i = 0; i < 0x0800; i++){
            ppuMap[index + i] = ppu->internalMemory + i;
        }
        index = 0x2800;
        //Mirror
        for(int i = 0; i < 0x0800; i++){
            ppuMap[index + i] = ppu->internalMemory + i;
        }
    }
    else{
        std::cout << "Vertikale Ausrichtung" << std::endl;
        index = 0x2000;
        for(int i = 0; i < 0x0400; i++){
            ppuMap[index + i] = ppu->internalMemory + i;
        }
        index = 0x2400;
        for(int i = 0; i < 0x0400; i++){
            ppuMap[index + i] = ppu->internalMemory + i;
        }

        index = 0x2800;
        for(int i = 0; i < 0x0400; i++){
            ppuMap[index + i] = ppu->internalMemory + i + 0x400;
        }
        index = 0x2C00;
        for(int i = 0; i < 0x0400; i++){
            ppuMap[index + i] = ppu->internalMemory + i + 0x400;
        }
    }

    if(mImpl!=nullptr) delete mImpl;

    switch(mappernum){
        case 0:
            mImpl = (AbstractMapper*) new Mapper0(this);
            break;
        case 2:
            mImpl = (AbstractMapper*) new Mapper2(this);
            break;
        default:
            mImpl = (AbstractMapper*) new Mapper0(this);
            break;
    }

    std::cout << "Mapper " << mappernum << " geladen." << std::endl;

}

uint8_t Mapper::read(uint8_t *address)
{
    [[unlikely]] if(memoryMap[(uintptr_t)address]==nullptr){
        return 0;
    }
    //wrap round?
    if((uintptr_t)address==0x10000) return *memoryMap[0];

    auto val = *memoryMap[(uintptr_t)address];
    // PPU-Callback
    if((uintptr_t)address >= 0x2000 && (uintptr_t)address <= 0x2007){
        return ppu->readRegister(memoryMap[(uintptr_t)address]);
    }
    if((uintptr_t)address == 0x4015){
        return apu->read((uintptr_t)address);
    }
    if((uintptr_t)address >= 0x4016 && (uintptr_t)address <= 0x4017){
        val = (controller_state[(uintptr_t)address & 0x0001] & 0x80) > 0;
        controller_state[(uintptr_t)address & 0x0001] <<= 1;
    }
    return val;
}

void Mapper::write(uint8_t *address, uint8_t value)
{
    [[unlikely]] if(memoryMap[(uintptr_t)address] == nullptr){
        return;
    }

    // PRG-ROM!
    // Das gibt aus irgendeinem Grund Probleme mit Mapper2 Spielen, muss ich mir ansehen. (Ah nee macht Sinn, weil du ja hier schreiben müssen musst um Pages zu wechseln)
    // Warum Kung Fu nicht mehr geht ist mir noch nicht klar
    // if((uintptr_t)address >= 0x8000 && (uintptr_t)address <= 0x10000) return;

    // PPU-Callback
    if((uintptr_t)address >= 0x2000 && (uintptr_t)address <= 0x2007){
        ppu->writeRegister(memoryMap[(uintptr_t)address], value);
        return;
    }

    // Controller
    if((uintptr_t)address == 0x4016){
        controller_state[(uintptr_t)address & 0x0001] = controller[(uintptr_t) address & 0x0001];
        return;
    }

    // APU
    if(((uintptr_t)address >= 0x4000 && (uintptr_t)address <= 0x4013) || ((uintptr_t)address == 0x4015) || ((uintptr_t)address == 0x4017)){
        apu->write((uintptr_t)address, value);
        return;
    }

    *memoryMap[(uintptr_t)address] = value;

    // OAMDMA write
    if((uintptr_t)address == 0x4014){
        //uint8_t cpuPage = read(address);
        uint16_t cpuAddress = ((uint16_t) value) << 8;
        for(int i=0; i < 256; i++){
            uint8_t val = read((uint8_t*)(uintptr_t)cpuAddress + i);
            ppu->pOAM[i] = val;
        }
        cpu->remainingCycles += 512; // Oder 514???
    }


    mImpl->writeRam(address, value);
}

uint8_t Mapper::readVRAM(uint8_t *address)
{
    if(ppuMap==nullptr) throw;
    [[unlikely]] if(ppuMap[(uintptr_t)address]==nullptr){
        return 0;
    }
    return *ppuMap[(uintptr_t)address];
}

void Mapper::writeVRAM(uint8_t *address, uint8_t value)
{
    [[unlikely]] if(ppuMap[(uintptr_t)address] == nullptr){
        return;
    }
    *ppuMap[(uintptr_t)address] = value;
}

void Mapper::pullNMI()
{
    cpu->pullNMI();
}

void Mapper::pullIRQ()
{
    cpu->pullIRQ();
}
