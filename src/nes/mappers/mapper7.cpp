#include "mapper7.h"

void Mapper7::writeRam(uint8_t *addr, uint8_t value)
{
    (void)addr;
    bankSelect = value & 0b111;
    mirror = value & 0b00010000 ? SINGLE_SCREEN_UPPER : SINGLE_SCREEN_LOWER;
}

uint8_t Mapper7::readRam(uint8_t *addr)
{
    if((uintptr_t)addr < 0x8000){
        return 0;
    }
    else return mapper.lock()->cart->prgRom[(uintptr_t)addr - 0x8000 + bankSelect * 0x8000];
}

Mapper7::Mapper7(std::shared_ptr<Mapper> m) : AbstractMapper(m)
{
    mirror = SINGLE_SCREEN_LOWER;
}

void Mapper7::reset()
{
    bankSelect = 0;
}
