#include "mapper3.h"

void Mapper3::writeRam(uint8_t *addr, uint8_t value)
{
    if((uintptr_t)addr < 0x8000){ // Prg-Ram
        // 2 KiB (0x800)
        prgRam[(((uintptr_t)addr - 0x6000) % 0x800)] = value;
    }
    else{ // Konfiguration
        if(mapper->cart->header.CHRROMSize <= 4){
            bankSelect = value & 0b11;
        }
        else{
            bankSelect = value & 0b1111;
        }
    }
}

uint8_t Mapper3::readRam(uint8_t *addr)
{
    if((uintptr_t)addr < 0x8000){ // PRG-Ram
        return prgRam[(((uintptr_t)addr - 0x6000) % 0x800)];
    }
    else{ // PRG-Rom
        if(mapper->cart->header.PRGROMSize == 1){
        // Gespiegelt
        return mapper->cart->prgRom[((uintptr_t)addr - 0x8000) % 0x4000];
        }
        else{
            //32KiB
            return mapper->cart->prgRom[(uintptr_t)addr - 0x8000];
        }
    }
}

void Mapper3::writePPU(uint8_t *addr, uint8_t value)
{
    if((uintptr_t)addr < 0x2000) // CHR
        return;

    else AbstractMapper::writePPU(addr, value);
}

uint8_t Mapper3::readPPU(uint8_t *addr)
{
    if((uintptr_t)addr < 0x2000) // CHR
        return mapper->cart->chrRom[(bankSelect * 0x2000) + (uintptr_t)addr];

    else return AbstractMapper::readPPU(addr);
}

Mapper3::Mapper3(std::shared_ptr<Mapper> m) : AbstractMapper(m)
{
    //2KiB
    prgRamSize = 0x800;
    prgRam = new uint8_t[prgRamSize];

    // Falls Persistenter PRG Ram
    AbstractMapper::loadSave();
}

Mapper3::~Mapper3()
{
    AbstractMapper::saveFile();
    delete[] prgRam;
}
