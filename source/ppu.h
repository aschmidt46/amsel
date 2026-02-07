#pragma once
#include <cstdint>
#include "screen.h"
#include <coroutine>
#include "mapper.h"
#include "palette.h"
#include <queue>
#include <memory>
#include <vector>
#include "ppu_types.h"


struct [[gnu::packed]] OAMSprite{
    uint8_t yPos = 0;   // Top of sprite + 1
    uint8_t tileIndex = 0;
    uint8_t attributes = 0;
    uint8_t xPos = 0;
};

struct renderState{
    int scanline = 0;
    int cycle = 0;
};

static_assert(sizeof(OAMSprite)==4);


class Screen;
class Mapper;
class Ppu{
    private:
    typedef void (Ppu::*ppuCmd) ();
    loopyReg v;
    loopyReg t;
    uint8_t fine_x = 0;
    bool w = false;

    const int numDots = 341 * 262;
    int ppuTiming = 0;

    std::vector<std::pair<std::vector<ppuCmd>, renderState>> timings;
    
    public:

    // Extern
    ctrlReg PPUCTRL;        //$2000
    maskReg PPUMASK;        //$2001
    statusReg PPUSTATUS;    //$2002
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
    uint8_t sprite_count = 0;
    uint8_t spriteShifterCHRLow[8];
    uint8_t spriteShifterCHRHigh[8];

    bool spriteZeroHitPossible = false;
    bool spriteZeroBeingRendered = false;


    uint8_t oamBuffer = 0;

    uint8_t* pOAM = (uint8_t*)OAM;

    

    // Schnittstelle
    std::shared_ptr<Mapper> mapper = nullptr;

    // Output, nicht Teil der PPU
    float* pixelBuffer;
    float* backBuffer;
    Palette pal;


    uint8_t vramReadBuffer = 0;

    uint8_t nextTileNTByte = 0x00;
    uint8_t nextTileATByte = 0x00;
    uint8_t nextTileCHRLow = 0x00;
    uint8_t nextTileCHRHigh = 0x00;

    uint16_t shifterCHRLow = 0x0000;
	uint16_t shifterCHRHigh = 0x0000;
	uint16_t shifterATLow  = 0x0000;
	uint16_t shifterATHigh  = 0x0000;


    Ppu() : pal("palette.pal") {
        for(int i = 0; i < 64; i++){
            OAM[i] = OAMSprite();
        }
        for(int i = 0; i < 8; i++){
            secondaryOAM[i] = OAMSprite();
            spriteShifterCHRLow[i] = 0;
            spriteShifterCHRHigh[i] = 0;
        }
        backBuffer = new float[256*240*3];
        pixelBuffer = new float[256*240*3];
        for(int i = 0; i < 256*240*3; i++){
            pixelBuffer[i] = 0;
            backBuffer[i] = 0;
        }
        internalMemory = new uint8_t[0x0800];
        for(int i = 0; i < 0x0800; i++){
            internalMemory[i] = 0;
        }
        palletteIndexes = new uint8_t[0x0020];
        for(int i = 0; i < 0x0020; i++){
            palletteIndexes[i] = 0;
        }
        fillTimings();
    };
    ~Ppu(){
        delete[] pixelBuffer;
        delete[] backBuffer;
        delete[] internalMemory;
        delete[] palletteIndexes;
    };
    void init(std::shared_ptr<Mapper> m){
        this->mapper = m;
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



