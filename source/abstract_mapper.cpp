#include "abstract_mapper.h"
#include "mapper.h"
#include "file_io.h"

uint8_t *AbstractMapper::translatePPUBus(uint8_t *addr)
{
    if((uintptr_t)addr<0x2000){ // CHR Rom
        return mapper->cart->chrRom + (uintptr_t)addr;
    }
    else if((uintptr_t)addr < 0x3F00){ // NT / AT + Mirror 0x3000-0x3EFF
        uint16_t a = (uintptr_t) addr - 0x2000;
        a = a % 0x1000; // Mirror
        uint16_t nametableNum = a / 0x400;
        uint16_t offset = a % 0x400;
        switch(mirror){
            case SINGLE_SCREEN_LOWER:{ // Ein Bildschirm, untere Bank
                return mapper->ppu->internalMemory +        offset; // Alle vier Nametables zeigen auf die untere interne PPU Bank
                break;
            }
            case SINGLE_SCREEN_UPPER:{ // Ein Bildschirm, obere Bank
                return mapper->ppu->internalMemory +        0x400 + offset; // Zweite Hälfte
                break;
            }
            case MIRROR_VERTICAL:{ // Vertical Mirror, Horizontal ausgelegt
                int bank = nametableNum % 2 == 0 ? 0 : 1; // 0,2 sind untere Bank, 1,3 obere
                return mapper->ppu->internalMemory +        offset + (bank * 0x400);
                break;
            }
            case MIRROR_HORIZONTAL:{ // Horizontal Mirror, Vertikal ausgelegt
                int bank = nametableNum < 2 ? 0 : 1; // 0,1 sind untere Bank, 2,3 obere
                return mapper->ppu->internalMemory +        offset + (bank * 0x400);
                break;
            }
            default:{ // Vier verschiedene Nametables
                if(a < 0x800)
                    return mapper->ppu->internalMemory + offset;
                else
                    return hardwiredVram + (offset - 0x800);
                break;
            }
        }
    }
    else{ // Paletten
        int p = (uintptr_t)addr - 0x3F00;
        return palletteMap[p];
    }
}

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
    this->mapper = m.get();
    mirror = mapper->cart->header.flags6.getNametableArrangement() ? MIRROR_VERTICAL : MIRROR_HORIZONTAL;
    palletteMap = new uint8_t*[0x100];

    // 2KiB + 2KiB intern in der PPU
    hardwiredVram = new uint8_t[0x800];
    if(mapper->cart->header.CHRROMSize == 0)
        chrRam = true;

    // Paletten sind immer fest, daher kann man die gut fest mappen
    for(int pi = 0; pi < 0x100; pi += 0x20){
        for(int i = 0; i < 0x20; i++){
            int j = i;

            // Innerhalb zeigen alle diese Register auf das gleiche innere Register (Backdrop Farbe)
            if (i == 0x0010) j = 0x0000;
	        if (i == 0x0014) j = 0x0004;
	        if (i == 0x0018) j = 0x0008;
	        if (i == 0x001C) j = 0x000C;

            palletteMap[i + pi] = mapper->ppu->palletteIndexes + j;
        }
    }
}

AbstractMapper::~AbstractMapper()
{
    delete[] hardwiredVram;
    delete[] palletteMap;
}

void AbstractMapper::reset(){}
