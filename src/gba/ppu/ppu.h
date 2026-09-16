#pragma once

#include "../arm/bus_types.h"
#include <utility>
#include <vector>
#include <memory>

#include "ppu_registers.h"
#include "../register/general_purpose.h"

namespace gba{
    class Bus;
    class PPU{
        std::vector<uint32_t> framebuffer;

        // Vram
        std::vector<Byte> paletteRam;
        std::vector<Byte> vRam;
        std::vector<Byte> oamAttribs;
        bool spriteAlphaOverride = false;
        // Aktuelle Sprites
        std::vector<OAMAttribs> oamAttribsCurrentLine;
        size_t oamAttribsCurrentLineSize = 0; // neu-Allokation verhindern größe von oamAttribsCurrentLine ist konstant

        // BG Priorität
        std::vector<PIXEL_T> layerOrder = std::vector<PIXEL_T>(6, PIXEL_T{});
        size_t layerOrderSize = 0;

        PIXEL_T getBackdrop();

        void setColorFromLayerOrder(const WINDOW_ACTIVES_T &actives);
        void mixFinalColor(const WINDOW_ACTIVES_T &actives, PIXEL_T &targetA, PIXEL_T &targetB, PIXEL_T &output);

        WINDOW_ACTIVES_T getActives(int window);
        WINDOW_ACTIVES_T getActives();
        bool insideObjectWindow = false;
        bool insideWindow0();
        bool insideWindow1();
        bool hasTargetA(int index);
        bool hasTargetB(int index);
        PIXEL_T blend(PIXEL_T &p1, PIXEL_T &p2);
        PIXEL_T brighten(const PIXEL_T &p);
        PIXEL_T darken(const PIXEL_T &p);
        

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

        WIN_H_T WINDOW_0_H = {.raw = 0};
        WIN_H_T WINDOW_1_H = {.raw = 0};
        WIN_V_T WINDOW_0_V = {.raw = 0};
        WIN_V_T WINDOW_1_V = {.raw = 0};
        WININ_T WININ = {.raw = 0};
        WINOUT_T WINOUT = {.raw = 0};
        HalfWord MOSAIC = 0;
    
        SPECIAL_EFFECTS_T SPECIAL_EFFECTS = {.raw = 0};
        ALPHA_BLEND_COEF_T ALPHA_BLENDING = {.raw = 0};
        BRIGHTNESS_FADE_T BRIGHTNESS_FADE = {.raw = 0};

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
        PPU(std::weak_ptr<Bus> bptr) : framebuffer(240*160, 0), paletteRam(0x400, 0), vRam(0x18000, 0), oamAttribs(0x400, 0), oamAttribsCurrentLine(128, OAMAttribs{}), bus(bptr){};
        uint32_t* accessFramebuffer();

        void clock();
        Word getVCount();

        bool hasFrame();

        void setPixel(int x, int y, uint32_t cr, uint32_t cg, uint32_t cb);

        void detectSpritesOnScanline();
        bool spriteCollidesCurrentPixel(const OAMAttribs &attrs);

        PIXEL_T drawSprites();
        PIXEL_T drawBG(const BGCNT_T &CONTROL, const HalfWord &BGX, const HalfWord &BGY, const int index);

        void insertBGIntoSorted(std::vector<int> &sorted, const int &element, int &oldSize);

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
