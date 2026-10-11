#pragma once
#include "../bus_types.h"
#include "2d/ppu.h"
#include "3d/gpu.h"

namespace nds{
    class SharedBus;

    union PageTableEntry{
        struct{
            Byte bankID : 7; // nur Bank-Index aus VRAM (0-40) (41 Bänke) (41 * 16 * 1024) = 0xA4000 (656KB)
            Byte valid : 1; // 1 - gemappt, 0 - nicht gemappt
        } state;
        Byte raw = 0;
    };

    union VRAMCNT_T{
        struct{
            Byte mst : 3;
            Byte ofs : 2;
            Byte _unused : 2;
            Byte enable : 1;
        } state;
        Byte raw = 0;
    };

    class LCD{

        SharedBus* bus;
        
        public:
        PPU engineA;
        PPU engineB;
        GPU engine3D;


        // 0x0600'0000 - 0x0700'0000 = 1024 * 16KB
        std::array<PageTableEntry, 1024> pageTable = {};
        std::array<PageTableEntry, 16> arm7PageTable = {};
        // ACHTUNG, [7] existiert nicht, [6] ist Bank G, [8] ist Bank H, [9] ist Bank I
        std::array<VRAMCNT_T, 10> vramControl = {};
        std::array<Byte, 0xA4000> vram = {};

        private:
        template<Word N>
        void switchVramBank();
        template<Word N>
        void clearVramBank();
        
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
        Word getVramOffset(Word addr);
        void writeMemory(Word addr, Byte val);
        Byte readMemoryNDS7(Word addr);
        void writeMemoryNDS7(Word addr, Byte val);

        void writeVramBankControl(Word addr, Byte val);
        Byte readVramBankControl(Word addr);
        Byte getVRAMSTAT();

        uint32_t* accessFramebuffer();
    };
    
    template<> void LCD::switchVramBank<0>();
    template<> void LCD::switchVramBank<1>();
    template<> void LCD::switchVramBank<2>();
    template<> void LCD::switchVramBank<3>();
    template<> void LCD::switchVramBank<4>();
    template<> void LCD::switchVramBank<5>();
    template<> void LCD::switchVramBank<6>();
    template<> void LCD::switchVramBank<8>();
    template<> void LCD::switchVramBank<9>();
    template<> void LCD::clearVramBank<0>();
    template<> void LCD::clearVramBank<1>();
    template<> void LCD::clearVramBank<2>();
    template<> void LCD::clearVramBank<3>();
    template<> void LCD::clearVramBank<4>();
    template<> void LCD::clearVramBank<5>();
    template<> void LCD::clearVramBank<6>();
    template<> void LCD::clearVramBank<8>();
    template<> void LCD::clearVramBank<9>();
}
