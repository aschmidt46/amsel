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
        nametableArrangement = mode & 0b00000011;
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
    if((uintptr_t)addr < 0x2000){ // Pattern Table / CHR Speicher
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
    else { // Nametables und Attribute tables
        uint16_t a = (uintptr_t) addr - 0x2000;
        uint16_t nametableNum = a / 0x400;
        uint16_t offset = a % 0x400;
        switch(nametableArrangement){
            case 0:{ // 0 - Ein Bildschirm, untere Bank
                return mapper->ppu->internalMemory +        offset; // Alle vier Nametables zeigen auf die untere interne PPU Bank
                break;
            }
            case 1:{ // 1 - Ein Bildschirm, obere Bank
                return mapper->ppu->internalMemory +        0x400 + offset; // Zweite Hälfte
                break;
            }
            case 2:{ // 2 - Vertical Mirror, Horizontal ausgelegt
                int bank = nametableNum % 2 == 0 ? 0 : 1; // 0,2 sind untere Bank, 1,3 obere
                return mapper->ppu->internalMemory +        offset + (bank * 0x400);
                break;
            }
            default:{ // 3 - Horizontal Mirror, Vertikal ausgelegt
                int bank = nametableNum < 2 ? 0 : 1; // 0,1 sind untere Bank, 2,3 obere
                return mapper->ppu->internalMemory +        offset + (bank * 0x400);
                break;
            }
        }
    }
}

// Consecutive read-modify-writes noch mal abchecken, ob das stimmt, was im WIki steht und ob das bei mir auch geht
void Mapper1::writeRam(uint8_t *addr, uint8_t value)
{
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

Mapper1::Mapper1(std::shared_ptr<Mapper> m) : AbstractMapper(m)
{
    // 32KiB PRG Ram
    prgRam = new uint8_t[0x8000];
    prgRamSize = 0x8000;


    // Standardmäßig erste Bank?
    int prgRamBank = 0;
    for(int i = 0; i < 0x2000; i++){
        mapper->memoryMap[0x6000 + i] = prgRam + i + (prgRamBank * 0x2000);
    }

    int prgRomBanks = mapper->cart->header.PRGROMSize;
    // Erste Prg Rom Bank
    for(int i = 0; i < 0x4000; i++){
        mapper->memoryMap[0x8000 + i] = mapper->cart->prgRom + i;
    }

    // Letzte Prg Rom Bank
    for(int i = 0; i < 0x4000; i++){
        mapper->memoryMap[0xC000 + i] = mapper->cart->prgRom + i + ((prgRomBanks-1) * 0x4000);
    }

    // Chr Rom bereits gesetzt in Mapper?

    AbstractMapper::loadSave();
}

Mapper1::~Mapper1()
{
    AbstractMapper::saveFile();
    delete[] prgRam;
}

uint8_t Mapper1::readRam(uint8_t *addr)
{
    switch(prgRomBankMode){
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

void Mapper1::writePPU(uint8_t *addr, uint8_t value)
{
    if((uintptr_t)addr < 0x3000){
        *translatePPUBus(addr) = value;
    }
    else AbstractMapper::writePPU(addr, value);
}

uint8_t Mapper1::readPPU(uint8_t *addr)
{
    if((uintptr_t)addr < 0x3000){
        return *translatePPUBus(addr);
    }
    else return AbstractMapper::readPPU(addr);
}
