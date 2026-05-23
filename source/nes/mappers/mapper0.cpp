#include "mapper0.h"
#include "../mapper.h"

void Mapper0::writeRam(uint8_t *addr, uint8_t value)
{
    return;
}

uint8_t Mapper0::readRam(uint8_t *addr)
{
    if((uintptr_t)addr < 0x8000)
        return 0;

    
    if(mapper.lock()->cart->header.PRGROMSize == 1){
        // Gespiegelt
        return mapper.lock()->cart->prgRom[((uintptr_t)addr - 0x8000) % 0x4000];
    }
    else{
        //32KiB
        return mapper.lock()->cart->prgRom[(uintptr_t)addr - 0x8000];
    }
}
