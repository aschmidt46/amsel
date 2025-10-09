#pragma once
#include <cstdint>
#include "screen.h"

struct [[gnu::packed]] OAMSprite{
    uint8_t yPos;   // Top of sprite + 1
    uint8_t tileIndex;
    uint8_t attributes;
    uint8_t xPos;
};

static_assert(sizeof(OAMSprite) == 4, "OAMSprite hat falsche Größe!");

class Screen;
class Ppu{
    private:
    uint8_t v,t,x,w;
    
    public:

    // Extern
    uint8_t PPUCTRL;
    uint8_t PPUMASK;
    uint8_t PPUSTATUS;
    uint8_t OAMADDR;
    uint8_t OAMDATA;
    uint8_t PPUSCROLL;
    uint8_t PPUADDR;
    uint8_t PPUDATA;

    uint8_t* internalMemory; // 2KB
    uint8_t* palletteIndexes; // 0x0020 Bytes
    OAMSprite* OAM; // 256 Bytes (64 * 4)

    Screen* screen;

    void clockPpu();
};


