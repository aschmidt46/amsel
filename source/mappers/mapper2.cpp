#include "mapper2.h"

void Mapper2::writeRam(uint8_t *addr, uint8_t value){
    if((uintptr_t)addr >= 0x8000)
        bankSelect = value & 0b00001111;
}

uint8_t Mapper2::readRam(uint8_t *addr)
{
    // Kein PRG-RAM
    if((uintptr_t)addr < 0x8000)
        return 0;

    
    if((uintptr_t)addr < 0xC000)
        return mapper->cart->prgRom[((uintptr_t)addr - 0x8000) + bankSelect * 0x4000];
    else
        return mapper->cart->prgRom[(((uintptr_t)addr - 0xC000) + mapper->cart->header.PRGROMSize * 0x4000 - 0x4000)];
}

Mapper2::Mapper2(std::shared_ptr<Mapper> m) : AbstractMapper(m) {}

void Mapper2::reset()
{
    bankSelect = 0;
}
