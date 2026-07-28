#pragma once

#include "arm/bus_types.h"
#include <vector>
#include <memory>

#include "ppu_registers.h"

namespace gba{
    class Bus;
    class PPU{
        std::vector<uint32_t> framebuffer;

        // Vram
        std::vector<Byte> paletteRam;
        std::vector<Byte> vRam;
        std::vector<Byte> oamAttribs;

        std::weak_ptr<Bus> bus;

        // Register
        LCDCONTROL_T LCDCONTROL = {.raw = 0};
        HalfWord GREENSWAP; // Undokumentiert
        LCDSTATUS_T LCDSTATUS = {.raw = 0};
        HalfWord currentScanline; //VCOUNT

        BGCNT_T BG0CONTROL = {.raw = 0};
        BGCNT_T BG1CONTROL = {.raw = 0};
        BGCNT_T BG2CONTROL = {.raw = 0};
        BGCNT_T BG3CONTROL = {.raw = 0};

        HalfWord BG0_X_OFFSET;
        HalfWord BG0_Y_OFFSET;
        HalfWord BG1_X_OFFSET;
        HalfWord BG1_Y_OFFSET;
        HalfWord BG2_X_OFFSET;
        HalfWord BG2_Y_OFFSET;
        HalfWord BG3_X_OFFSET;
        HalfWord BG3_Y_OFFSET;

        HalfWord BG2_DX;
        HalfWord BG2_DMX;
        HalfWord BG2_DY;
        HalfWord BG2_DMY;
        Word     BG2_REFERENCE_X;
        Word     BG2_REFERENCE_Y;

        HalfWord BG3_DX;
        HalfWord BG3_DMX;
        HalfWord BG3_DY;
        HalfWord BG3_DMY;
        Word     BG3_REFERENCE_X;
        Word     BG3_REFERENCE_Y;

        HalfWord WINDOW_0_HORIZONTAL_DIM;
        HalfWord WINDOW_1_HORIZONTAL_DIM;
        HalfWord WINDOW_0_VERTICAL_DIM;
        HalfWord WINDOW_1_VERTICAL_DIM;
        HalfWord WININ;
        HalfWord WINOUT;
        HalfWord MOSAIC;
    
        HalfWord SPECIAL_EFFECTS;
        HalfWord ALPHA_BLENDING;
        HalfWord BRIGHTNESS_FADE;

        Word currentCycle = 0;

        bool hasframe = false;
        
        public:
        PPU() = default;
        PPU(std::weak_ptr<Bus> bptr) : framebuffer(240*160, 0), paletteRam(0x400, 0), vRam(0x18000, 0), oamAttribs(0x400, 0), bus(bptr){};
        uint32_t* accessFramebuffer();

        void clock();
        Word getVCount();

        bool hasFrame();

        void setPixel(int x, int y, uint32_t cr, uint32_t cg, uint32_t cb);

        void drawSprites();
        void drawBG0();

        void drawPixelMode0();
        void drawPixelMode1();
        void drawPixelMode2();
        void drawPixelMode3();
        void drawPixelMode4();
        void drawPixelMode5();
        
        void writePPURegister(Word addr, Byte val);
        Byte readPPURegister(Word addr);

        void writePPUMemory(Word addr, Byte value);
        Byte readPPUMemory(Word addr);
    };
}
