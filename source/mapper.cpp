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
    // Nur unterer Adressbereich, weil der fest ist
    memoryMap = new uint8_t*[0x6000];
    this->ppu = ppu;
    this->cpu = cpu;
    this->apu = apu;

    controller[0] = 0;
    controller[1] = 0;
    controller_state[0] = 0;
    controller_state[1] = 0;

    for(unsigned int i = 0; i < 0x6000; i++){
        memoryMap[i] = nullptr;
    }

    int offset = 0;
    // Interner Ram (2KiB) + 3 Mirrors vom internen Ram
    for(int j = 0; j < 4; j++){
        for(int i = 0; i < 0x0800; i++){
            memoryMap[offset + i] = cpu->internalMemory + i;
        }
        offset += 0x0800;
    }
    // Je 8 PPU Register ab 0x2000 bis < 0x4000 aufwärts (Mirror)
    for(int i = offset; i < 0x4000; i+=0x8){
        memoryMap[i]   = (uint8_t*)&ppu->PPUCTRL;
        memoryMap[i+1] = (uint8_t*)&ppu->PPUMASK;
        memoryMap[i+2] = (uint8_t*)&ppu->PPUSTATUS;
        memoryMap[i+3] = (uint8_t*)&ppu->OAMADDR;
        memoryMap[i+4] = (uint8_t*)&ppu->OAMDATA;
        memoryMap[i+5] = (uint8_t*)&ppu->PPUSCROLL;
        memoryMap[i+6] = (uint8_t*)&ppu->PPUADDR;
        memoryMap[i+7] = (uint8_t*)&ppu->PPUDATA;
    }

    // IO Register (nicht implementiert)
    io = new uint8_t[0x18];
    for(int i = 0; i < 0x18; i++){
        io[i] = 0;
    }

    offset = 0x4000;
    for(int i = 0; i < 0x18; i++){
         memoryMap[i + offset] = io + i;
    }
    offset += 0x18;

    //APU und I/O Teile, die normalerweise nicht an sind
    offset += 0x8;

    // PRG-Ram, etc...
    // Übernimmt Mapper-Implementierung
}

bool Mapper::changeCart(NESFile *cartridge)
{
    this->cart = cartridge;
    int mappernum = cart->header.getMapper();

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
    //wrap round?
    if((uintptr_t)address==0x10000) return *memoryMap[0];
    
    if((uintptr_t)address>= 0x6000)
        return mImpl->readRam(address);
    
    [[unlikely]] if(memoryMap[(uintptr_t)address]==nullptr){
        return 0;
    }
    
    // Feste Register
    // PPU-Callback
    if((uintptr_t)address >= 0x2000 && (uintptr_t)address <= 0x3FFF){
        return ppu->readRegister(memoryMap[(uintptr_t)address]);
    }
    if((uintptr_t)address == 0x4015){
        return apu->read((uintptr_t)address);
    }
    auto val = *memoryMap[(uintptr_t)address];
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

    if((uintptr_t)address >= 0x6000){
        mImpl->writeRam(address, value);
        return;
    }

    [[unlikely]] if(memoryMap[(uintptr_t)address] == nullptr){
        return;
    }

    // // PRG-"ROM"!
    // if((uintptr_t)address >= 0x8000 && (uintptr_t)address <= 0x10000) return;

    // PPU-Callback
    if((uintptr_t)address >= 0x2000 && (uintptr_t)address <= 0x3FFF){
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
    *memoryMap[(uintptr_t)address] = value;
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

void Mapper::riseA12()
{
    mImpl->onPPUA12RisingEdge();
}
