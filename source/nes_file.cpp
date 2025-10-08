#include "nes_file.h"

NAMETABLE_ARRANGEMENT Flags6::getNametableArrangement()
{
    return (NAMETABLE_ARRANGEMENT)(data & 1u);
}

bool Flags6::containsBatteryPackedPRG()
{
    return data & 2u;
}

bool Flags6::containsTrainer()
{
    return data & 4u;
}

bool Flags6::hasAlternativeNametableLayout()
{
    return data & 8u;
}

bool Flags7::VSUnisystem()
{
    return data & 1u;
}

bool Flags7::PlayChoice10()
{
    return data & 2u;
}

bool Flags7::isNES2()
{
    return 2 == ((data & 0b00001100) >> 2);
}

TVSystem Flags10::getTVSystem()
{
    unsigned int val = data & 3u;
    if(val==0) return NTSC;
    if(val==2) return PAL;
    return DUAL_COMP;
}

bool Flags10::isPresentPRGRAM()
{
    return data & 0b00010000;
}

bool Flags10::boardHasBusConflicts()
{
    return data & 0b00100000;
}

unsigned char NESHeader::getMapper()
{
    return (flags7.data & 0b11110000) | ((flags6.data & 0b11110000) >> 4);
}

unsigned char NESHeader::getPRGRamSize()
{
    return flags8;
}
