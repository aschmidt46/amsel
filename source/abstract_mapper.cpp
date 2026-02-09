#include "abstract_mapper.h"
#include "mapper.h"

void AbstractMapper::writePPU(uint8_t *addr, uint8_t value)
{
    [[unlikely]] if(mapper->ppuMap[(uintptr_t)addr] == nullptr){
        return;
    }

    // Falls kein character RAM, darf hier nicht geschrieben werden!
    if((uintptr_t)addr <= 0x2000 && !mapper->chrRAM)
        return;

    *(mapper->ppuMap[(uintptr_t)addr]) = value;
}

uint8_t AbstractMapper::readPPU(uint8_t *addr)
{
    [[unlikely]] if(mapper->ppuMap[(uintptr_t)addr]==nullptr){
        return 0;
    }
    return *(mapper->ppuMap[(uintptr_t)addr]);
}
