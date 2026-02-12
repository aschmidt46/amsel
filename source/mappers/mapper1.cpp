#include "mapper1.h"

void Mapper1::shiftInto(bool value)
{
    shiftReg = (value << 7) | (shiftReg >> 1); // Wert wird von links reingeshiftet
}

bool Mapper1::shiftIsFull()
{
    
    return shiftReg & 8; // Nach vier Shifts
}

void Mapper1::changeBanks(uint16_t bank, uint8_t mode)
{
    if(bank < 0x8000) return;
    if(bank < 0xA000){ // 0x8000 - 0x9FFF: Control
        chrRomBankMode =       mode & 0b00010000;
        prgRomBankMode =      (mode & 0b00001100) >> 2;
        int nametableArrangement = mode & 0b00000011;
        if(nametableArrangement==0)         mirror = SINGLE_SCREEN_LOWER;
        else if(nametableArrangement==1)    mirror = SINGLE_SCREEN_UPPER;
        else if(nametableArrangement==2)    mirror = MIRROR_VERTICAL;
        else                                mirror = MIRROR_HORIZONTAL;
    }
    else if(bank < 0xC000){ // 0xA000 - 0xBFFF: CHR Bank 0
        //Ram ist 2* 8KiB;
        int chrSize = 2;
        if(mapper->cart->header.CHRROMSize>0)
            chrSize = mapper->cart->header.CHRROMSize;
        
        if(chrSize==2){
            chrBank0 = mode & 1; // Es gibt nur zwei Bänke, bit entscheidet, ob erste oder zweite
        }
        else{
            chrBank0 = mode;
        }
    }
    else if(bank < 0xE000){ // 0xC000 - 0xDFFF: CHR Bank 1
        //if(chrRomBankMode) //ignoriert in 8KiB Modus
        chrBank1 = mode;
    }
    else{ // 0xE000 - 0xFFFF: PRG Bank
        enablePRGRam = mode & 0b00010000;
        prgRomBankSelect = mode & 0b00001111; // unterstes Bit ignoriert wenn PrgRomBankMode auf 32 KiB Modus (0,1)
    }
}

uint8_t *Mapper1::translatePPUBus(uint8_t *addr)
{
    // Pattern Table / CHR Speicher (0x0000 - 0x2000)
    if(chrRomBankMode){ // 4KiB Modus
        if((uintptr_t)addr < 0x1000){ // Bank 0
            return mapper->cart->chrRom +       (uintptr_t)addr + (chrBank0 * 0x1000);
        }
        else{ // Bank 1
            return mapper->cart->chrRom +       (uintptr_t)addr - 0x1000 + (chrBank1 * 0x1000);
        }
    }
    else{ // 8KiB Modus
        return mapper->cart->chrRom +           (uintptr_t)addr + ((chrBank0 & 0b11111110) * 0x1000); // Unterstes Bit ignoriert
    }
}

// Consecutive read-modify-writes noch mal abchecken, ob das stimmt, was im WIki steht und ob das bei mir auch geht
void Mapper1::writeRam(uint8_t *addr, uint8_t value)
{
    if((uintptr_t)addr < 0x8000){ // PRG-Ram
        // aktuell festgesetzt auf 8KiB RAM (nullte Bank)
        prgRam[((uintptr_t)addr - 0x6000) + (0 * 0x2000)] = value;
    }
    else{ // Konfiguration über serielle Schnittstelle
        if(value & 0b10000000){
        shiftReg = 0b10000000; // Detektions Eins
        prgRomBankMode = 3; // Reset, letzte Bank fixen
    }
    else{
        bool bit0 = value & 1;
        if(shiftIsFull()){
            shiftInto(bit0);
            uint8_t value = (shiftReg & 0b11111000) >> 3; // Eins fallen lassen
            shiftReg = 0b10000000; // Shiftreg reset
            uint16_t internalSelect = ((uint16_t)(uintptr_t)addr) & 0b1110000000000000; // bit 13 und 14, oberstes Bit mitnehmen, weil Adresse überhalb von 0x8000 liegt
            changeBanks(internalSelect, value);
        }
        else{
            shiftInto(bit0);
        }
    }
    }
}

Mapper1::Mapper1(std::shared_ptr<Mapper> m) : AbstractMapper(m)
{
    // 32KiB PRG Ram
    prgRamSize = 0x8000;
    prgRam = new uint8_t[prgRamSize];
    for(int i = 0; i < prgRamSize; i++){
        prgRam[i] = 0;
    }

    AbstractMapper::loadSave();
}

Mapper1::~Mapper1()
{
    AbstractMapper::saveFile();
    delete[] prgRam;
}

void Mapper1::reset()
{
    shiftReg = 0;
    chrRomBankMode = false;
    prgRomBankMode = 3;

    // Chr banks
    chrBank0 = 0;
    chrBank1 = 1;

    // Prg Bank
    enablePRGRam = false;
    prgRomBankSelect = 0;
}

uint8_t Mapper1::readRam(uint8_t *addr)
{
    if((uintptr_t)addr < 0x8000){ // PRG-Ram
        // aktuell festgesetzt auf 8KiB RAM (nullte Bank)
        return prgRam[((uintptr_t)addr - 0x6000) + (0 * 0x2000)];
    }
    else{
        switch(prgRomBankMode){ //PRG-ROM
            case 2:{ // Bank bei 0x8000 fest auf erste Bank, hintere getauscht
                if((uintptr_t) addr < 0xC000){
                    return mapper->cart->prgRom[(uintptr_t)addr - 0x8000];
                }
                else{
                    return mapper->cart->prgRom[(uintptr_t)addr - 0xC000 + (prgRomBankSelect * 0x4000)];
                }
                break;
            }
            case 3:{ // Bank bei 0xC000 fest auf letzte Bank, vordere getauscht
                if((uintptr_t) addr < 0xC000){
                    return mapper->cart->prgRom[(uintptr_t)addr - 0x8000 + (prgRomBankSelect * 0x4000)];
                }
                else{
                    return mapper->cart->prgRom[(uintptr_t)addr - 0xC000 + ((mapper->cart->header.PRGROMSize-1) * 0x4000)];
                }
                break;
            }
            default:{ // 0,1 (32 KiB Mode), 32 KiB bei 0x8000 getauscht
                return mapper->cart->prgRom[(uintptr_t)addr - 0x8000 + ((prgRomBankSelect & 0b11111110) * 0x4000)];
                break;
            }
        }
    }
}

void Mapper1::writePPU(uint8_t *addr, uint8_t value)
{
    if((uintptr_t)addr < 0x2000){
        *translatePPUBus(addr) = value;
    }
    else AbstractMapper::writePPU(addr, value);
}

uint8_t Mapper1::readPPU(uint8_t *addr)
{
    if((uintptr_t)addr < 0x2000){
        return *translatePPUBus(addr);
    }
    else return AbstractMapper::readPPU(addr);
}
