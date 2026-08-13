#include "ppu.h"
#include "bus.h"
#include "framework/stringlib.h"
#include "gba/arm/bus_types.h"
#include "ppu_registers.h"
#include <algorithm>
#include <iostream>
#include <array>
#include <utility>

using namespace gba;

uint32_t *gba::PPU::accessFramebuffer()
{
    return framebuffer.data();
}

void gba::PPU::clock() {
    currentCycle++;
    if(currentCycle == 1004){
        // Hblank
        LCDSTATUS.state.hBlankFlag = 1;
        if(LCDSTATUS.state.hBlankIE && currentScanline < 160){
            bus.lock()->setIF(1, true);
        }
        bus.lock()->PPUEnteredHBlank();
    }
    else if(currentCycle >= 1232){
        currentCycle = 0;
        currentScanline++;
        LCDSTATUS.state.hBlankFlag = 0;
        if(currentScanline == LCDSTATUS.state.vCountSetting && LCDSTATUS.state.vCounterIE){
            bus.lock()->setIF(2, true);
            LCDSTATUS.state.vCounterFlag = 1;
        }
        bus.lock()->PPULeftHBlank();
    }
    if(currentScanline == 160 && currentCycle == 0){
        // Vblank
        LCDSTATUS.state.vBlankFlag = 1;
        if(LCDSTATUS.state.vBlankIE){
            // std::cout << "Vblank IRQ" << std::endl;
            bus.lock()->setIF(0, true);
        }
        hasframe = true;
        bus.lock()->PPUEnteredVBlank();
    }
    else if(currentScanline >= 228){
        currentScanline = 0;
        LCDSTATUS.state.vCounterFlag = 0;
    }
    if(currentScanline >= 227){
        LCDSTATUS.state.vBlankFlag = 0;
        bus.lock()->PPULeftVBlank();
    }

    if(currentCycle < 240 && currentScanline < 160){ // Bis jetzt k.A. wie das Timing wirklich ist
        switch(LCDCONTROL.state.bgMode){
            case 0:
                drawPixelMode0();
                break;
            case 1:
                drawPixelMode1();
                break;
            case 2:
                drawPixelMode2();
                break;
            case 3:
                drawPixelMode3();
                break;
            case 4:
                drawPixelMode4();
                break;
            case 5:
                drawPixelMode5();
                break;
        }
    }
}

gba::Word gba::PPU::getVCount(){
    return currentScanline;
}

void gba::PPU::detectSpritesOnScanline(){
    
}

void PPU::drawSprites(){
    // beide Blöcke
    // int increment = 
    // for(Word i = 0x06010000; i < 0x06018000; i+=increment){

    // }
}

Word PPU::seIndexFast(Word tx, Word ty, BGCNT_T bgcnt)
{
    Word n = tx + ty * 32;
    if(tx >= 32)
        n += 0x03E0;
    if(ty >= 32 && (bgcnt.state.screenSize)==3)
        n += 0x0400;
    return n;
}


constexpr std::array<std::pair<Word, Word>, 4> regularBgrSizes = {std::pair{256,256}, std::pair{512,256}, std::pair{256,512}, std::pair{512,512}};


void PPU::drawBG(const BGCNT_T &CONTROL, const HalfWord &BGX, const HalfWord &BGY){
    const int pixelX = currentCycle;
    const int pixelY = currentScanline;
//    Memory	0600:0000	0600:4000	0600:8000	0600:C000
// charblock        0	        1	        2	        3
// screenblock	0	…	7	8	…	15	16	…	23	24	…	31

    const Word screenblockSize = 0x800;
    const Word chrblockSize = 0x4000;
    const Word screenBaseBlock = CONTROL.state.screenBaseBlock;
    const Word characterBaseBlock = CONTROL.state.CHRBaseBlock;
    auto [bgrX, bgrY] = regularBgrSizes[CONTROL.state.screenSize];
    Byte* entries = vRam.data() + (screenBaseBlock * screenblockSize);
    Byte* chrEntries = vRam.data() + (characterBaseBlock * chrblockSize);
    bool bpp8 = CONTROL.state.colorsPalettes;
    const Word tileWidth = bpp8 ? 0x40 : 0x20;
    const Word tileWidthByte = bpp8 ? 8 : 4;
    Word mapX = pixelX + (BGX & 0x1FF);
    Word mapY = pixelY + (BGY & 0x1FF);
    mapX %= bgrX;
    mapY %= bgrY;
    Word tileCoordX = mapX / 8;
    Word tileCoordY = mapY / 8;
    Word sbb= seIndexFast(tileCoordX, tileCoordY, CONTROL);
    Word inTileX = mapX % 8;
    Word inTileY = mapY % 8;
    Word screenIndex = sbb;
    Byte entryLow = entries[screenIndex * 2];
    Byte entryHigh = entries[screenIndex * 2 + 1];
    ScreenEntry entry;
    entry.raw = entryLow | (HalfWord(entryHigh) << 8);
    Word tileID = entry.state.TileID;
    Byte* tileStart = chrEntries + tileWidth * tileID;
    // Flipping:

    const Word verticalFactor = entry.state.flipVertical ? -1 : 1;
    const Word horizontalFactor = entry.state.flipHorizontal ? -1 : 1;

    const Word verticalWidth = entry.state.flipVertical ? 7 * tileWidthByte : 0;
    const Word horizontalWidth = entry.state.flipHorizontal ? (bpp8 ? 7 : 3) : 0;

    const Word bitWidthInTile = bpp8 ? inTileX : inTileX / 2;

    // 8px x 4 bit = 32 bit = 4 byte, bei 8bbp 8 byte
    const Word pixelIndex = (verticalWidth + verticalFactor * inTileY * tileWidthByte) + (horizontalWidth + horizontalFactor * bitWidthInTile);
    if(tileStart + pixelIndex >= vRam.data() + vRam.size()) return;
    Byte pixel = *(tileStart + pixelIndex);
    if(!bpp8){
        if((inTileX & 1) ^ entry.state.flipHorizontal) pixel >>= 4;
        pixel &= 0xF;
    }
    Byte paletteIndex =  bpp8 ? pixel : pixel | (entry.state.paletteBank << 4);
    HalfWord colorLow = paletteRam[2 * Word(paletteIndex)]; // Byte adressiert
    HalfWord colorHigh = paletteRam[2 * Word(paletteIndex) + 1];
    HalfWord color = colorLow | (colorHigh << 8);
    HalfWord red = color & 0b11111;
    HalfWord green = (color >> 5) & 0b11111;
    HalfWord blue = (color >> 10) & 0b11111;
    if(paletteIndex > 0)
        setPixel(pixelX, pixelY, red << 3, green << 3, blue << 3);
}

bool gba::PPU::hasFrame() {
    bool tmp = hasframe;
    hasframe = false;
    return tmp;
}

void gba::PPU::setPixel(int x, int y, uint32_t cr, uint32_t cg, uint32_t cb)
{
    int index = (x + 240 * y);
    #ifndef BUILD_LIBRETRO_CORE
    uint32_t col = (255 << 24) | (cb << 16) | (cg << 8) | (cr);
    #else
    uint32_t col = (255 << 24) | (cb) | (cg << 8) | (cr << 16);
    #endif
    framebuffer[index] = col;
}

void gba::PPU::drawPixelMode0() {
    setPixel(currentCycle, currentScanline, 0, 0, 0);
    std::array<Word, 4> bgOrder = {0, 1, 2, 3};
    std::stable_sort(std::begin(bgOrder), std::end(bgOrder), [this](const auto &a, const auto &b){
        return BG_CNT[a].state.BGPriority < BG_CNT[b].state.BGPriority;
    });
    
    for(const auto &i : bgOrder){
        if(displayBG(i)){
            drawBG(BG_CNT[i], BG_X_OFFSET[i], BG_Y_OFFSET[i]);
        }
    }
}

void gba::PPU::drawPixelMode1() {
    setPixel(currentCycle, currentScanline, 0, 0, 255);
}

void gba::PPU::drawPixelMode2() {
    setPixel(currentCycle, currentScanline, 255, 255, 255);
}

void gba::PPU::drawPixelMode3() {
    int index = currentCycle + 240 * currentScanline;
    HalfWord pixel = HalfWord(vRam[2 * index]) | (HalfWord(vRam[2 * index + 1]) << 8);
    HalfWord red = pixel & 0b11111;
    HalfWord green = (pixel >> 5) & 0b11111;
    HalfWord blue = (pixel >> 10) & 0b11111;
    setPixel(currentCycle, currentScanline, red << 3, green << 3, blue << 3);
}

void gba::PPU::drawPixelMode4() {
    int index = currentCycle + 240 * currentScanline;
    size_t page = LCDCONTROL.state.frameSelect ? 0xA000 : 0;
    Byte paletteIndex = vRam[index + page];
    HalfWord pixel = HalfWord(paletteRam[2 * paletteIndex]) | (HalfWord(paletteRam[2 * paletteIndex + 1]) << 8);
    HalfWord red = pixel & 0b11111;
    HalfWord green = (pixel >> 5) & 0b11111;
    HalfWord blue = (pixel >> 10) & 0b11111;
    setPixel(currentCycle, currentScanline, red << 3, green << 3, blue << 3);
}

void gba::PPU::drawPixelMode5() {
    setPixel(currentCycle, currentScanline, 0, 255, 0);
}

void gba::PPU::writePPURegister(Word addr, Byte val) {
    if(addr == 0x04000000){
        LCDCONTROL.raw = (LCDCONTROL.raw & 0xFF00) | val;
        LCDCONTROL.state.cgbMode = 0;
    }
    else if(addr == 0x04000001){
        LCDCONTROL.raw = (LCDCONTROL.raw & 0x00FF) | (HalfWord(val) << 8);
        LCDCONTROL.state.cgbMode = 0;
    }
    else if(addr == 0x04000004){
        LCDSTATUS.raw = (LCDSTATUS.raw & 0xFF00) | val;
    }
    else if(addr == 0x04000005){
        LCDSTATUS.raw = (LCDSTATUS.raw & 0x00FF) | (HalfWord(val) << 8);
    }

    else if(addr == 0x04000008){
        BG_CNT[0].raw = (BG_CNT[0].raw & 0xFF00) | val;
    }
    else if(addr == 0x04000009){
        BG_CNT[0].raw = (BG_CNT[0].raw & 0x00FF) | (HalfWord(val) << 8);
    }

    else if(addr == 0x0400000A){
        BG_CNT[1].raw = (BG_CNT[1].raw & 0xFF00) | val;
    }
    else if(addr == 0x0400000B){
        BG_CNT[1].raw = (BG_CNT[1].raw & 0x00FF) | (HalfWord(val) << 8);
    }

    else if(addr == 0x0400000C){
        BG_CNT[2].raw = (BG_CNT[2].raw & 0xFF00) | val;
    }
    else if(addr == 0x0400000D){
        BG_CNT[2].raw = (BG_CNT[2].raw & 0x00FF) | (HalfWord(val) << 8);
    }

    else if(addr == 0x0400000E){
        BG_CNT[3].raw = (BG_CNT[3].raw & 0xFF00) | val;
    }
    else if(addr == 0x0400000F){
        BG_CNT[3].raw = (BG_CNT[3].raw & 0x00FF) | (HalfWord(val) << 8);
    }

    else if(addr == 0x04000010){
        BG_X_OFFSET[0] = (BG_X_OFFSET[0] & 0xFF00) | val;
    }
    else if(addr == 0x04000011){
        BG_X_OFFSET[0] = (BG_X_OFFSET[0] & 0xFF) | (HalfWord(val) << 8);
    }
    else if(addr == 0x04000012){
        BG_Y_OFFSET[0] = (BG_Y_OFFSET[0] & 0xFF00) | val;
    }
    else if(addr == 0x04000013){
        BG_Y_OFFSET[0] = (BG_Y_OFFSET[0] & 0xFF) | (HalfWord(val) << 8);
    }

    else if(addr == 0x04000014){
        BG_X_OFFSET[1] = (BG_X_OFFSET[1] & 0xFF00) | val;
    }
    else if(addr == 0x04000015){
        BG_X_OFFSET[1] = (BG_X_OFFSET[1] & 0xFF) | (HalfWord(val) << 8);
    }
    else if(addr == 0x04000016){
        BG_Y_OFFSET[1] = (BG_Y_OFFSET[1] & 0xFF00) | val;
    }
    else if(addr == 0x04000017){
        BG_Y_OFFSET[1] = (BG_Y_OFFSET[1] & 0xFF) | (HalfWord(val) << 8);
    }

    else if(addr == 0x04000018){
        BG_X_OFFSET[2] = (BG_X_OFFSET[2] & 0xFF00) | val;
    }
    else if(addr == 0x04000019){
        BG_X_OFFSET[2] = (BG_X_OFFSET[2] & 0xFF) | (HalfWord(val) << 8);
    }
    else if(addr == 0x0400001A){
        BG_Y_OFFSET[2] = (BG_Y_OFFSET[2] & 0xFF00) | val;
    }
    else if(addr == 0x0400001B){
        BG_Y_OFFSET[2] = (BG_Y_OFFSET[2] & 0xFF) | (HalfWord(val) << 8);
    }

    else if(addr == 0x0400001C){
        BG_X_OFFSET[3] = (BG_X_OFFSET[3] & 0xFF00) | val;
    }
    else if(addr == 0x0400001D){
        BG_X_OFFSET[3] = (BG_X_OFFSET[3] & 0xFF) | (HalfWord(val) << 8);
    }
    else if(addr == 0x0400001E){
        BG_Y_OFFSET[3] = (BG_Y_OFFSET[3] & 0xFF00) | val;
    }
    else if(addr == 0x0400001F){
        BG_Y_OFFSET[3] = (BG_Y_OFFSET[3] & 0xFF) | (HalfWord(val) << 8);
    }
}

gba::Byte gba::PPU::readPPURegister(Word addr)
{
    switch(addr){
        case 0x04000000:
            return LCDCONTROL.raw;
        case 0x04000001:
            return LCDCONTROL.raw >> 8;
        case 0x04000004:
            return LCDSTATUS.raw;
        case 0x04000005:
            return LCDSTATUS.raw >> 8;
        case 0x04000006:
            return currentScanline;
        case 0x04000007:
            return currentScanline >> 8;
        case 0x04000008:
            return BG_CNT[0].raw;
        case 0x04000009:
            return BG_CNT[0].raw >> 8;
        case 0x0400000A:
            return BG_CNT[1].raw;
        case 0x0400000B:
            return BG_CNT[1].raw >> 8;
        case 0x0400000C:
            return BG_CNT[2].raw;
        case 0x0400000D:
            return BG_CNT[2].raw >> 8;
        case 0x0400000E:
            return BG_CNT[3].raw;
        case 0x0400000F:
            return BG_CNT[3].raw >> 8;
    }

    std::cout << "Unbekannter PPU Register read: "<< getHex0x(addr, 8) << "\n";
    return 0;
}

// MUSS gefixt werden (auch für Openlara), bei Byte Writes wird das Byte in beide Bytes des Halbworts geschrieben
void gba::PPU::writePPUMemory(Word addr, Byte value) {
    if(addr >= 0x05000000 && addr < 0x06000000){
        auto mod = (addr - 0x05000000) % 0x400;
        paletteRam[mod] = value;
    }
    else if(addr >= 0x06000000 && addr < 0x07000000){
        // auto relAddr = (addr - 0x06000000);
        auto mod = addr % 0x20000;
        if(mod >= 0x18000) mod -= 0x8000;
        vRam[mod] = value;
    }
    else if(addr >= 0x07000000 && addr < 0x08000000){
        auto mod = (addr - 0x07000000) % 0x400;
        oamAttribs[mod] = value;
    }
}

gba::Byte gba::PPU::readPPUMemory(Word addr)
{
    if(addr >= 0x05000000 && addr < 0x06000000){
        auto mod = (addr - 0x05000000) % 0x400;
        return paletteRam[mod];
    }
    else if(addr >= 0x06000000 && addr < 0x07000000){
        // auto relAddr = (addr - 0x06000000);
        auto mod = addr % 0x20000;
        if(mod >= 0x18000) mod -= 0x8000;
        return vRam[mod];
    }
    else if(addr >= 0x07000000 && addr < 0x08000000){
        auto mod = (addr - 0x07000000) % 0x400;
        return oamAttribs[mod];
    }
    std::cout << "Unbekannter PPU mem read\n";
    return 0;
}
