#pragma once
#include "../bus_types.h"
#include "2d/ppu.h"

namespace nds{
    class SharedBus;
    class LCD{

        SharedBus* bus;

        PPU engineA;
        PPU engineB;

        LCDSTATUS_T DISPSTAT = {.raw = 0}; // engine A + B
        bool hasframe = false;
        
        public:
        Word vCount = 0; // engine A + B
        LCD(SharedBus* bus) : bus(bus){};
        void renderScanline();
        
        void onHBlank();
        void onVBlank();
        void increment();

        Word getVCount();

        bool hasFrame();
    };
}
