#include "mapper.h"
#include <assert.h>
#include <iostream>
#include <algorithm>
#include <format>
#include "framework/file_io.h"


Mapper::Mapper(std::weak_ptr<Cpu> cpu, std::weak_ptr<Ppu> ppu, std::weak_ptr<Apu> apu)
{
    this->ppu = ppu;
    this->cpu = cpu;
    this->apu = apu;

    controller[0] = 0;
    controller[1] = 0;
    controller_state[0] = 0;
    controller_state[1] = 0;
}

bool Mapper::changeCart(std::shared_ptr<NESFile> cartridge)
{
    this->cart = cartridge;
    int mappernum = cart->header.getMapper();

    auto m = getMapper(shared_from_this(), mappernum);
    if(!m.has_value())
        return false;
    else mapperImplementation = std::move(m.value());

    return true;

}

void Mapper::connectController(std::weak_ptr<Controller> controller1, std::weak_ptr<Controller> controller2)
{
    this->controller1 = controller1;
    this->controller2 = controller2;
    controller1.lock()->init(shared_from_this());
    controller2.lock()->init(shared_from_this());
}

uint8_t *Mapper::getMemoryMapping(uint8_t* addr)
{
    uint16_t add = (uintptr_t)addr;

    // Interner Ram (2KiB) + 3 Mirrors vom internen Ram
    if(add < 0x2000){
        uint16_t offset = add % 0x0800;
        return cpu.lock()->internalMemory + offset;
    }

    // Je 8 PPU Register ab 0x2000 bis < 0x4000 aufwärts (Mirror)
    if(add < 0x4000){
        uint16_t offset = add % 0x8;
        switch(offset){
            case 0:
                return (uint8_t*)&ppu.lock()->PPUCTRL;
            case 1:
                return (uint8_t*)&ppu.lock()->PPUMASK;
            case 2:
                return (uint8_t*)&ppu.lock()->PPUSTATUS;
            case 3:
                return &ppu.lock()->OAMADDR;
            case 4:
                return &ppu.lock()->OAMDATA;
            case 5:
                return &ppu.lock()->PPUSCROLL;
            case 6:
                return &ppu.lock()->PPUADDR;
            default:
                return &ppu.lock()->PPUDATA;
        }
    }

    // Wird nie benutzt, aber nötig, damit die Funktion nicht nullptr zurückgibt
    if(add < 0x4018){
        return &io[add - 0x4000];
    }
    
    // PRG-Ram, etc...
    // Übernimmt Mapper-Implementierung
    return nullptr;
}

uint8_t Mapper::read(uint8_t *address)
{
    //wrap round?
    if((uintptr_t)address==0x10000) return *getMemoryMapping(0);
    if((uintptr_t)address>= 0x6000)
        return mapperImplementation->readRam(address);
    
    [[unlikely]] if(getMemoryMapping(address)==nullptr){
        return 0;
    }
    
    // Feste Register
    // PPU-Callback
    if((uintptr_t)address >= 0x2000 && (uintptr_t)address <= 0x3FFF){
        return ppu.lock()->readRegister(getMemoryMapping(address));
    }
    if((uintptr_t)address == 0x4015){
        return apu.lock()->read((uintptr_t)address);
    }
    auto val = *getMemoryMapping(address);
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
        mapperImplementation->writeRam(address, value);
        return;
    }

    [[unlikely]] if(getMemoryMapping(address) == nullptr){
        return;
    }

    // // PRG-"ROM"!
    // if((uintptr_t)address >= 0x8000 && (uintptr_t)address <= 0x10000) return;

    // PPU-Callback
    if((uintptr_t)address >= 0x2000 && (uintptr_t)address <= 0x3FFF){
        ppu.lock()->writeRegister(getMemoryMapping(address), value);
        return;
    }

    // Controller
    if((uintptr_t)address == 0x4016){
        controller_state[0] = controller[0];
        controller_state[1] = controller[1];
        return;
    }

    // APU
    if(((uintptr_t)address >= 0x4000 && (uintptr_t)address <= 0x4013) || ((uintptr_t)address == 0x4015) || ((uintptr_t)address == 0x4017)){
        apu.lock()->write((uintptr_t)address, value);
        return;
    }

    
    // OAMDMA write
    if((uintptr_t)address == 0x4014){
        //uint8_t cpuPage = read(address);
        uint16_t cpuAddress = ((uint16_t) value) << 8;
        for(int i=0; i < 256; i++){
            uint8_t val = read((uint8_t*)(uintptr_t)cpuAddress + i);
            ppu.lock()->pOAM[i] = val;
        }
        cpu.lock()->remainingCycles += 512; // Oder 514???
    }
    *getMemoryMapping(address) = value;
}

uint8_t Mapper::readVRAM(uint8_t *address)
{
    return mapperImplementation->readPPU(address);
}

void Mapper::writeVRAM(uint8_t *address, uint8_t value)
{
    mapperImplementation->writePPU(address, value);
}

void Mapper::pullNMI()
{
    cpu.lock()->pullNMI();
}

void Mapper::pullIRQ()
{
    cpu.lock()->pullIRQ();
}

void Mapper::riseA12()
{
    mapperImplementation->onPPUA12RisingEdge();
}

void Mapper::reset()
{
    if(mapperImplementation)
        mapperImplementation->reset();
}

bool Mapper::canSave()
{
    return this->mapperImplementation->canSave();
}

std::vector<uint8_t> Mapper::getSaveData()
{
    return this->mapperImplementation->getSaveData();
}
