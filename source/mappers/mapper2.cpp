#include "mapper2.h"

void Mapper2::writeRam(uint8_t *addr, uint8_t value){
    if((uintptr_t)addr >= 0x8000 && (uintptr_t)addr <= 0xFFFF){
        uint8_t bankSelect = value & 0b00001111;
        for(int i = 0x8000; i < 0xC000; i++){
            mapper->memoryMap[i] = mapper->cart->prgRom + ((i - 0x8000) + bankSelect * 0x4000);
        }
    }
}

Mapper2::Mapper2(std::shared_ptr<Mapper> m)
{
    this->mapper = m;

    int index = 0;
    for (int i = 0x8000; i < 0xC000; i++)
    {
        mapper->memoryMap[i] = mapper->cart->prgRom + (index + mapper->cart->header.PRGROMSize * 0x4000 - 0x4000);
        index++;
    }
    index = 0;
    for (int i = 0xC000; i <= 0xFFFF; i++)
    {
        mapper->memoryMap[i] = mapper->cart->prgRom + (index + mapper->cart->header.PRGROMSize * 0x4000 - 0x4000);
        index++;
    }
}
