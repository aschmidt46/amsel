#include "abstract_mapper.h"
#include "../mapper.h"
#include "framework/file_io.h"
#include <cstring>

uint8_t *AbstractMapper::translatePPUBus(uint8_t *addr)
{
    if((uintptr_t)addr<0x2000){ // CHR Rom
        return mapper.lock()->cart->chrRom.data() + (uintptr_t)addr;
    }
    else if((uintptr_t)addr < 0x3F00){ // NT / AT + Mirror 0x3000-0x3EFF
        uint16_t a = (uintptr_t) addr - 0x2000;
        a = a % 0x1000; // Mirror
        uint16_t nametableNum = a / 0x400;
        uint16_t offset = a % 0x400;
        switch(mirror){
            case SINGLE_SCREEN_LOWER:{ // Ein Bildschirm, untere Bank
                return mapper.lock()->ppu.lock()->internalMemory.data() +        offset; // Alle vier Nametables zeigen auf die untere interne PPU Bank
                break;
            }
            case SINGLE_SCREEN_UPPER:{ // Ein Bildschirm, obere Bank
                return mapper.lock()->ppu.lock()->internalMemory.data() +        0x400 + offset; // Zweite Hälfte
                break;
            }
            case MIRROR_VERTICAL:{ // Vertical Mirror, Horizontal ausgelegt
                int bank = nametableNum % 2 == 0 ? 0 : 1; // 0,2 sind untere Bank, 1,3 obere
                return mapper.lock()->ppu.lock()->internalMemory.data() +        offset + (bank * 0x400);
                break;
            }
            case MIRROR_HORIZONTAL:{ // Horizontal Mirror, Vertikal ausgelegt
                int bank = nametableNum < 2 ? 0 : 1; // 0,1 sind untere Bank, 2,3 obere
                return mapper.lock()->ppu.lock()->internalMemory.data() +        offset + (bank * 0x400);
                break;
            }
            default:{ // Vier verschiedene Nametables
                if(a < 0x800)
                    return mapper.lock()->ppu.lock()->internalMemory.data() + offset;
                else
                    return hardwiredVram + (offset - 0x800);
                break;
            }
        }
    }
    else{ // Paletten
        int i = (uintptr_t)addr - 0x3F00;
        int j = i % 0x20;

        // Innerhalb zeigen alle diese Register auf das gleiche innere Register (Backdrop Farbe)
        if (i == 0x0010) j = 0x0000;
	    if (i == 0x0014) j = 0x0004;
	    if (i == 0x0018) j = 0x0008;
	    if (i == 0x001C) j = 0x000C;

        return mapper.lock()->ppu.lock()->palletteIndexes + j;
    }
}

void AbstractMapper::loadSave()
{
    containsBatteryBackedPRGRAM = mapper.lock()->cart->header.flags6.containsBatteryBackedPRG();
    name = mapper.lock()->cart->name;

    if(containsBatteryBackedPRGRAM){
        #ifdef BUILD_DESKTOP
        if(!FileIO::getInstance().createSave(name)){
            FileIO::getInstance().loadSave(name, prgRam.data(), prgRamSize);
        }
        #endif
    }
}

void AbstractMapper::saveFile()
{
    if(containsBatteryBackedPRGRAM){
        #ifdef BUILD_DESKTOP
        FileIO::getInstance().saveData(name, prgRam.data(), prgRamSize);
        #endif
    }
}

void AbstractMapper::writePPU(uint8_t *addr, uint8_t value)
{
    // Falls kein character RAM, darf hier nicht geschrieben werden!
    if((uintptr_t)addr <= 0x2000 && !chrRam)
        return;

    if((uintptr_t)addr < 0x4000){ // CHR, NT/AT, Paletten
        *translatePPUBus(addr) = value;
    }
}

uint8_t AbstractMapper::readPPU(uint8_t *addr)
{
    if((uintptr_t)addr < 0x4000){ // CHR, NT/AT, Paletten
        return *translatePPUBus(addr);
    }
    else return 0; // Open Bus?
}

AbstractMapper::AbstractMapper(std::shared_ptr<Mapper> m)
{
    this->mapper = m;
    mirror = mapper.lock()->cart->header.flags6.getNametableArrangement() ? MIRROR_VERTICAL : MIRROR_HORIZONTAL;

    if(mapper.lock()->cart->header.CHRROMSize == 0)
        chrRam = true;
}

AbstractMapper::~AbstractMapper()
{
}

void AbstractMapper::reset(){}

bool AbstractMapper::canSave()
{
    return this->containsBatteryBackedPRGRAM;
}

std::vector<uint8_t> AbstractMapper::getSaveData()
{
    std::vector<uint8_t> sv(prgRamSize);
    std::memcpy(sv.data(), prgRam.data(), sv.size());
    return sv;
}
