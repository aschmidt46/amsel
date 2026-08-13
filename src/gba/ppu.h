#pragma once

#include "arm/bus_types.h"
#include <utility>
#include <vector>
#include <memory>

#include "ppu_registers.h"
#include "register/general_purpose.h"

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
        HalfWord GREENSWAP = 0; // Undokumentiert
        LCDSTATUS_T LCDSTATUS = {.raw = 0};
        HalfWord currentScanline = 0; //VCOUNT

        BGCNT_T BG_CNT[4] = {{.raw = 0}, {.raw = 0}, {.raw = 0}, {.raw = 0}};

        HalfWord BG_X_OFFSET[4] = {0, 0, 0, 0};
        HalfWord BG_Y_OFFSET[4] = {0, 0, 0, 0};

        HalfWord BG2_DX = 0;
        HalfWord BG2_DMX = 0;
        HalfWord BG2_DY = 0;
        HalfWord BG2_DMY = 0;
        Word     BG2_REFERENCE_X = 0;
        Word     BG2_REFERENCE_Y = 0;

        HalfWord BG3_DX = 0;
        HalfWord BG3_DMX = 0;
        HalfWord BG3_DY = 0;
        HalfWord BG3_DMY = 0;
        Word     BG3_REFERENCE_X = 0;
        Word     BG3_REFERENCE_Y = 0;

        HalfWord WINDOW_0_HORIZONTAL_DIM = 0;
        HalfWord WINDOW_1_HORIZONTAL_DIM = 0;
        HalfWord WINDOW_0_VERTICAL_DIM = 0;
        HalfWord WINDOW_1_VERTICAL_DIM = 0;
        HalfWord WININ = 0;
        HalfWord WINOUT = 0;
        HalfWord MOSAIC = 0;
    
        HalfWord SPECIAL_EFFECTS = 0;
        HalfWord ALPHA_BLENDING = 0;
        HalfWord BRIGHTNESS_FADE = 0;

        Word currentCycle = 0;

        bool hasframe = false;

        Word seIndexFast(Word tx, Word ty, BGCNT_T bgcnt);

        inline bool displayBG(const int i) const{
            switch(i){
                case 0:
                    return LCDCONTROL.state.displayBG0;
                case 1:
                    return LCDCONTROL.state.displayBG1;
                case 2:
                    return LCDCONTROL.state.displayBG2;
                case 3:
                    return LCDCONTROL.state.displayBG3;
                default:
                    std::unreachable();
            }
        }
        
        public:
        PPU() = default;
        PPU(std::weak_ptr<Bus> bptr) : framebuffer(240*160, 0), paletteRam(0x400, 0), vRam(0x18000, 0), oamAttribs(0x400, 0), bus(bptr){};
        uint32_t* accessFramebuffer();

        void clock();
        Word getVCount();

        bool hasFrame();

        void setPixel(int x, int y, uint32_t cr, uint32_t cg, uint32_t cb);

        void detectSpritesOnScanline();

        void drawSprites();
        void drawBG(const BGCNT_T &CONTROL, const HalfWord &BGX, const HalfWord &BGY);

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
