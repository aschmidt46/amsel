#pragma once
#include <vector>

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
    unsigned char data;

    NAMETABLE_ARRANGEMENT getNametableArrangement();
    bool containsBatteryPackedPRG();
    bool containsTrainer();
    bool hasAlternativeNametableLayout();
};

struct Flags7{
    unsigned char data;

    bool VSUnisystem();
    bool PlayChoice10();
    bool isNES2();
};

struct Flags10{
    unsigned char data;

    TVSystem getTVSystem();
    bool isPresentPRGRAM();
    bool boardHasBusConflicts();
};

struct NESHeader{
    char head[4];
    unsigned char PRGROMSize; //16KiB units
    unsigned char CHRROMSize; // 8KiB units
    Flags6 flags6;
    Flags7 flags7;
    unsigned char flags8; //prg ram size, bei 0 sind es 8kib
    unsigned char flags9; //reserved 0
    Flags10 flags10;
    char unused[5];

    unsigned char getMapper();
    unsigned char getPRGRamSize();
};

struct NESFile{
    NESHeader header;
    std::vector<unsigned char> trainer; // 0 oder 512
    std::vector<unsigned char> prgRom; // 16384 * x
    std::vector<unsigned char> chrRom; // 8192 * y
    std::vector<unsigned char> playchoiceInstRom; // 0 oder 8192
    std::vector<unsigned char> playchoicePRom; // 0 oder 32
    std::vector<unsigned char> footer;
};
