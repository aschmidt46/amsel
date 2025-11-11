#pragma once
#include <cstdint>
#include "screen.h"
#include <coroutine>
#include "mapper.h"
#include "palette.h"
#include <queue>
#include <memory>
#include <vector>


union loopy_register {
    uint16_t value = 0x0000;
    struct{
        // Coarse X (tile column) (Bits 0-4)
        uint16_t coarse_x : 5;

        // Coarse Y (tile row) (Bits 5-9)
        uint16_t coarse_y : 5;

        // Nametable select (Bits 10-11)
        uint16_t name_table_x : 1;
        uint16_t name_table_y : 1;

        // Fine Y offset (Vertical offset within a tile) (Bits 12-14)
        uint16_t fine_y : 3;
        uint16_t unused : 1;
    };
};

union ctrlreg {
    uint8_t value;
    struct{
        uint8_t nametable_x : 1;
        uint8_t nametable_y : 1;
        uint8_t increment_mode : 1;
        uint8_t pattern_sprite : 1;
        uint8_t pattern_background : 1;
        uint8_t sprite_size : 1;
        uint8_t unused : 1;
        uint8_t enable_nmi : 1;
    };
};

union maskreg
	{
        uint8_t reg;
		struct
		{
			uint8_t grayscale : 1;
			uint8_t render_background_left : 1;
			uint8_t render_sprites_left : 1;
			uint8_t render_background : 1;
			uint8_t render_sprites : 1;
			uint8_t enhance_red : 1;
			uint8_t enhance_green : 1;
			uint8_t enhance_blue : 1;
		};

	};

    union statusreg
	{
		struct
		{
			uint8_t unused : 5;
			uint8_t sprite_overflow : 1;
			uint8_t sprite_zero_hit : 1;
			uint8_t vertical_blank : 1;
		};

		uint8_t reg;
	};

struct [[gnu::packed]] OAMSprite{
    uint8_t yPos;   // Top of sprite + 1
    uint8_t tileIndex;
    uint8_t attributes;
    uint8_t xPos;
};

struct renderState{
    int scanline;
    int cycle;
};

static_assert(sizeof(OAMSprite)==4);


class Screen;
class Mapper;
class Ppu{
    private:
    typedef void (Ppu::*ppuCmd) ();
    loopy_register v;
    loopy_register t;
    uint8_t fine_x = 0;
    bool w = false;

    const int numDots = 341 * 262;
    int ppuTiming = 0;

    std::vector<std::pair<std::vector<ppuCmd>, renderState>> timings;
    
    public:

    // Extern
    ctrlreg PPUCTRL;        //$2000
    maskreg PPUMASK;        //$2001
    statusreg PPUSTATUS;    //$2002
    uint8_t OAMADDR = 0;    //$2003
    uint8_t OAMDATA = 0;    //$2004
    uint8_t PPUSCROLL = 0;  //$2005
    uint8_t PPUADDR = 0;    //$2006
    uint8_t PPUDATA = 0;    //$2007

    // Intern
    uint8_t* internalMemory; // 2KB
    uint8_t* palletteIndexes; // 0x0020 Bytes
    //uint8_t* OAM; // 256 Bytes (64 * 4)
    OAMSprite OAM[64];
    OAMSprite secondaryOAM[8];
    uint8_t sprite_count;
    uint8_t spriteShifterCHRLow[8];
    uint8_t spriteShifterCHRHigh[8];

    bool spriteZeroHitPossible = false;
    bool spriteZeroBeingRendered = false;


    uint8_t oamBuffer;

    uint8_t* pOAM = (uint8_t*)OAM;

    

    // Schnittstelle
    Screen* screen;
    Mapper* mapper;

    // Output, nicht Teil der PPU
    float* pixelBuffer;
    float* backBuffer;
    Palette pal;


    uint8_t vramReadBuffer;

    uint8_t nextTileNTByte = 0x00;
    uint8_t nextTileATByte = 0x00;
    uint8_t nextTileCHRLow = 0x00;
    uint8_t nextTileCHRHigh = 0x00;

    uint16_t shifterCHRLow = 0x0000;
	uint16_t shifterCHRHigh = 0x0000;
	uint16_t shifterATLow  = 0x0000;
	uint16_t shifterATHigh  = 0x0000;


    Ppu() : pal("palette.pal") {
        backBuffer = new float[256*240*3];
        pixelBuffer = new float[256*240*3];
        for(int i = 0; i < 256*240*3; i++){
            pixelBuffer[i] = 0;
            backBuffer[i] = 0;
        }
        internalMemory = new uint8_t[0x0800];
        palletteIndexes = new uint8_t[0x0020];
        fillTimings();
    };
    ~Ppu(){
        delete[] pixelBuffer;
        delete[] backBuffer;
        delete[] internalMemory;
        delete[] palletteIndexes;
    };
    void init(Mapper* m, Screen* s){
        mapper = m;
        screen = s;
    };

    bool blanking = true;
    bool frameReady = false;

    // Pixelposition
	int16_t scanline = 0;
	int16_t cycle = 0;

    bool unevenFrame = true;
    void clock();
    void setPixel(int x, int y, glm::vec3 c);
    void swapBuffers();

    // Callbacks
    void writeRegister(uint8_t* reg, uint8_t val);
    uint8_t readRegister(uint8_t* reg);

    void fillTimings();
    void incrementX();
    void incrementY();
    void resetX();
    void resetY();
    void loadBackgroundShifters();
    void updateShifters();
    void clearFlags();
    void readNTByte();
    void readATByte();
    void readCHRByteLow();
    void readCHRByteHigh();
    void evaluateSprites();
    void setSpriteShifters();
    void pullNMI();
    void renderPixel();

};



