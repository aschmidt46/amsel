#include "mapper.h"
#include <assert.h>
#include <iostream>
#include <algorithm>
#include <format>
#include "file_io.h"

std::string mhex(uintptr_t input){
    std::string str = std::format("{:x}", input);
    std::transform(str.begin(), str.end(), str.begin(), ::toupper);
    return str;
}


Mapper::Mapper(Cpu* cpu, Ppu* ppu, Apu* apu)
{
    memoryMap = new uint8_t*[ADDRSPACE];
    this->ppu = ppu;
    this->cpu = cpu;
    this->apu = apu;

    controller[0] = 0;
    controller[1] = 0;
    controller_state[0] = 0;
    controller_state[1] = 0;

    // zu groß, aber egal?
    // ppuMap = new uint8_t*[ADDRSPACE];
    for(unsigned int i = 0; i < ADDRSPACE; i++){
        memoryMap[i] = nullptr;
        // ppuMap[i] = nullptr;
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
        io[i] = 0;
    }
    for(int i = 0; i < 0x18; i++){
         memoryMap[i + index] = io + i;
    }
    index += 0x18;

    //APU und I/O Teile, die normalerweise nicht an sind
    index += 0x8;


    //PRG-RAM 8KB
    index = 0x6000;
    prgRam = new uint8_t[0x2000];
    for(int i = 0; i < 0x2000; i++){
        prgRam[i] = 0;
    }
    for(int i = 0; i < 0x2000; i++){
        memoryMap[index + i] = prgRam + i;
    }



    // Palletten gespiegelt bis 0x4000
    // for(index = 0x3F00; index < 0x4000; index += 0x0020){
    //     for(int i = 0; i < 0x20; i++){
    //         auto pIndex = ppu->palletteIndexes;
    //         int j = i;
    //         // Innerhalb zeigen alle diese Register auf das gleiche innere Register (Backdrop Farbe)
    //         if (i == 0x0010) j = 0x0000;
	//         if (i == 0x0014) j = 0x0004;
	//         if (i == 0x0018) j = 0x0008;
	//         if (i == 0x001C) j = 0x000C;
            
    //         ppuMap[index + i] = pIndex + j;
    //     }
    // }
}

bool Mapper::changeCart(NESFile *cartridge)
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
    // assert(cartridge->header.CHRROMSize == 0 || cartridge->header.CHRROMSize == 1);
    if(cartridge->header.CHRROMSize == 0)
        chrRAM = true;

    eject();

    auto m = getMapper(shared_from_this(), mappernum);
    if(!m.has_value())
        return false;
    else mImpl = m.value();

    return true;

}

void Mapper::eject()
{
    if(mImpl!=nullptr){
        delete mImpl;
        mImpl = nullptr;
    }
}

void Mapper::connectController(Controller *controller)
{
    this->controller1 = controller;
    controller1->init(shared_from_this());
}

uint8_t Mapper::read(uint8_t *address)
{
    [[unlikely]] if(memoryMap[(uintptr_t)address]==nullptr){
        return 0;
    }
    //wrap round?
    if((uintptr_t)address==0x10000) return *memoryMap[0];

    if((uintptr_t)address>= 0x8000)
        return mImpl->readRam(address);

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
    // wrap round
    if((uintptr_t)address==0x10000) address = 0;
    [[unlikely]] if(memoryMap[(uintptr_t)address] == nullptr){
        return;
    }
    if((uintptr_t)address >= 0x8000){
        mImpl->writeRam(address, value);
        return;
    }

    // PRG-"ROM"!
    if((uintptr_t)address >= 0x8000 && (uintptr_t)address <= 0x10000) return;

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
}

uint8_t Mapper::readVRAM(uint8_t *address)
{
    return mImpl->readPPU(address);
}

void Mapper::writeVRAM(uint8_t *address, uint8_t value)
{
    mImpl->writePPU(address, value);
}

void Mapper::pullNMI()
{
    cpu->pullNMI();
}

void Mapper::pullIRQ()
{
    cpu->pullIRQ();
}
