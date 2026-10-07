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
        std::vector<uint32_t> framebuffer;
        Word vCount = 0; // engine A + B
        LCD(SharedBus* bus) : bus(bus), framebuffer(256*192*2){};
        void init();
        void renderScanline();
        
        void onHBlank();
        void onVBlank();
        void increment();

        Word getVCount();

        bool hasFrame();

        Byte readRegister(Word addr);
        void writeRegister(Word addr, Byte val);
        Byte readMemory(Word addr);
        void writeMemory(Word addr, Byte val);

        uint32_t* accessFramebuffer();
    };
}
