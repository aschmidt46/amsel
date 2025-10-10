#include "nes_file.h"
#include <fstream>
#include <assert.h>
#include <iostream>
#include <string>
#include <format>
#include <algorithm>

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

uint8_t NESHeader::getMapper()
{
    return (flags7.data & 0b11110000) | ((flags6.data & 0b11110000) >> 4);
}

uint8_t NESHeader::getPRGRamSize()
{
    return flags8;
}

NESHeader NESHeader::createHeader(uint8_t *data)
{
    NESHeader header;
    for(int i = 0; i < 4; i++){
        header.head[i] = data[i];
    }
    header.PRGROMSize = data[4];
    header.CHRROMSize = data[5];
    header.flags6.data = data[6];
    header.flags7.data = data[7];
    header.flags8 = data[8];
    header.flags9 = data[9];
    header.flags10.data = data[10];
    for(int i = 0; i < 5; i++){
        header.unused[i] = data[11+i];
    }
    return header;
}

NESFile::NESFile(const char *path)
{
    std::ifstream stream(path, std::ios::in | std::ios::binary);
    std::vector<uint8_t> contents((std::istreambuf_iterator<char>(stream)), std::istreambuf_iterator<char>());
    

    rawData = new uint8_t[contents.size()];
    for(int i = 0; i < contents.size(); i++){
        rawData[i] = contents[i];
    }
    stream.close();
    
    header = NESHeader::createHeader(rawData);
    unsigned int index = 16;

    if(header.flags6.containsTrainer()){
        trainer = rawData + index;
        index += 512;
    }
    else trainer = nullptr;

    int prgroms = (int)header.PRGROMSize * 16384;
    if(prgroms == 0){
        prgRom = nullptr;
    }
    else{
        prgRom = rawData + index;
        index += prgroms;
    }

    int chrroms = (int)header.CHRROMSize * 8192;
    if(chrroms == 0){
        chrRom = nullptr;
    }
    else{
        chrRom = rawData + index;
        index += chrroms;
    }

    // muss erstmal reichen...

    assert(chrRom != nullptr);
    assert(this->prgRom != nullptr);
}
