#include "abstract_mapper.h"
#include "mapper.h"
#include "file_io.h"

void AbstractMapper::loadSave()
{
    containsBatteryBackedPRGRAM = mapper->cart->header.flags6.containsBatteryBackedPRG();
    name = mapper->cart->name;

    if(containsBatteryBackedPRGRAM){
        if(!FileIO::getInstance().createSave(name)){
            FileIO::getInstance().loadSave(name, prgRam, prgRamSize);
        }
    }
}

void AbstractMapper::saveFile()
{
    if(containsBatteryBackedPRGRAM){
        FileIO::getInstance().saveData(name, prgRam, prgRamSize);
    }
}

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

AbstractMapper::AbstractMapper(std::shared_ptr<Mapper> m)
{
    this->mapper = m;
}
