#pragma once
#include <vector>
#include <cstdint>

enum NAMETABLE_ARRANGEMENT{
    VERTICAL = 0,
    HORIZONTAL = 1
};

enum TVSystem{
    NTSC,
    PAL,
    DUAL_COMP
};

struct Flags6{
    uint8_t data;

    NAMETABLE_ARRANGEMENT getNametableArrangement();
    bool containsBatteryPackedPRG();
    bool containsTrainer();
    bool hasAlternativeNametableLayout();
};

struct Flags7{
    uint8_t data;

    bool VSUnisystem();
    bool PlayChoice10();
    bool isNES2();
};

struct Flags10{
    uint8_t data;

    TVSystem getTVSystem();
    bool isPresentPRGRAM();
    bool boardHasBusConflicts();
};

struct NESHeader{
    uint8_t head[4];
    uint8_t PRGROMSize; //16KiB units
    uint8_t CHRROMSize; // 8KiB units
    Flags6 flags6;
    Flags7 flags7;
    uint8_t flags8; //prg ram size, bei 0 sind es 8kib
    uint8_t flags9; //reserved 0
    Flags10 flags10;
    uint8_t unused[5];

    uint8_t getMapper();
    uint8_t getPRGRamSize();
    static NESHeader createHeader(uint8_t* data);
};

struct NESFile{
    NESHeader header;
    uint8_t* trainer; // 0 oder 512
    uint8_t* prgRom; // 16384 * x
    uint8_t* chrRom; // 8192 * y
    uint8_t* playchoiceInstRom; // 0 oder 8192
    uint8_t* playchoicePRom; // 0 oder 32
    uint8_t* footer;

    uint8_t* rawData;

    NESFile(const char* path);
    ~NESFile(){delete[] rawData;};
};
