#include "ppu.h"
#include "../bus.h"
#include "framework/stringlib.h"
#include "gba/arm/bus_types.h"
#include "ppu_registers.h"
#include <algorithm>
#include <iostream>
#include <array>
#include <utility>

using namespace gba;

template<typename T> // Insertion Sort für EIN Element in bereits sortiertes Array
void insertIntoSorted(std::vector<T> &sorted, const T &element, size_t &oldSize){
    sorted[oldSize] = element;
    size_t i = oldSize;
    oldSize++;
    while(i > 0 && sorted[i] < sorted[i-1]){
        std::swap(sorted[i], sorted[i-1]);
        i--;
    }
}

// Alternative: Index-basiert für Hintergründe
void PPU::insertBGIntoSorted(std::vector<int> &sorted, const int &element, int &oldSize){
    sorted[oldSize] = element;
    size_t i = oldSize;
    oldSize++;
    while(i > 0 && BG_CNT[sorted[i]] < BG_CNT[sorted[i-1]]){
        std::swap(sorted[i], sorted[i-1]);
        i--;
    }
}

WINDOW_ACTIVES_T PPU::getActives(int window){
    HalfWord currentWindowHW = 0;
    if(window < 2)
        currentWindowHW = WININ.raw;
    else
        currentWindowHW = WINOUT.raw;

    size_t offset = 0;
    if(window % 2 != 0)
        offset = 8;
    
    currentWindowHW >>= offset;

    return WINDOW_ACTIVES_T{
        .enableBG0 = (currentWindowHW & 1) > 0,
        .enableBG1 = (currentWindowHW & 2) > 0,
        .enableBG2 = (currentWindowHW & 4) > 0,
        .enableBG3 = (currentWindowHW & 8) > 0,
        .enableObj = (currentWindowHW & 16) > 0,
        .enableSpecialFX = (currentWindowHW & 32) > 0
    };
}

bool PPU::insideWindow0(){
    return currentCycle >= WINDOW_0_H.state.leftMost && currentCycle < WINDOW_0_H.state.rightMostPlus1
    && currentScanline >= WINDOW_0_V.state.topMost && currentScanline < WINDOW_0_V.state.bottomMostPlus1;
}

bool PPU::insideWindow1(){
    return currentCycle >= WINDOW_1_H.state.leftMost && currentCycle < WINDOW_1_H.state.rightMostPlus1
    && currentScanline >= WINDOW_1_V.state.topMost && currentScanline < WINDOW_1_V.state.bottomMostPlus1;
}

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

// Shape, Size -> sizeX, sizeY in px
constexpr std::array<std::array<std::pair<size_t, size_t>, 4>, 3> spriteShapeSizeTable = {{
    {std::pair{8,8}, std::pair{16,16},std::pair{32,32}, std::pair{64,64}},
    {std::pair{16,8}, std::pair{32,8}, std::pair{32,16}, std::pair{64,32}},
    {std::pair{8,16}, std::pair{8,32}, std::pair{16,32}, std::pair{32,64}}
}};

void PPU::detectSpritesOnScanline(){
    this->oamAttribsCurrentLineSize = 0;
    const Word y = currentScanline;

    // sicher?
    OAMAttribs* oamMap = (OAMAttribs*)this->oamAttribs.data();
    // Rückwärtsiteration fixt die Sprite Priorität, nochmal anschauen
    constexpr int oamSize = 128;
    for(int i = oamSize-1; i >= 0; i--){
        OAMAttribs current = oamMap[i];
        auto [sizeX, sizeY] = spriteShapeSizeTable[current.attr0.state.spriteShape][current.attr1.state.spriteSize];
        (void)sizeX;
        const int yEnd = current.attr0.state.yCoord + sizeY;
        if((current.attr0.state.yCoord <= y && yEnd > int(y)) || (current.attr0.state.yCoord > 160 && (yEnd - 256) > int(y))){
            //push
            insertIntoSorted(oamAttribsCurrentLine, current, oamAttribsCurrentLineSize);
        }
    }
}

bool PPU::spriteCollidesCurrentPixel(const OAMAttribs &current){
    const Word x = currentCycle;
    const Word y = currentScanline;

    auto [sizeX, sizeY] = spriteShapeSizeTable[current.attr0.state.spriteShape][current.attr1.state.spriteSize];
    const int xEnd = current.attr1.state.xCoord + sizeX;
    const int yEnd = current.attr0.state.yCoord + sizeY;
    if(
        ((current.attr0.state.yCoord <= y && yEnd > int(y)) || (current.attr0.state.yCoord > 160 && (yEnd - 256) > int(y)))
        && ((current.attr1.state.xCoord <= x && xEnd > int(x)) || (current.attr1.state.xCoord > 240 && (xEnd - 512) > int(x)))
    ){
        return true;
    }
    return false;
}

PIXEL_T PPU::drawSprites(){
    PIXEL_T result;
    result.priority = 99;
    bool spriteMappingMode1D = LCDCONTROL.state.ObjCharVRAMMapping;
    insideObjectWindow = false;


    OAMAttribs* oamMap = this->oamAttribsCurrentLine.data();
    const size_t oamSize = oamAttribsCurrentLineSize;

    OAMAttribs sprite = {};

    for(size_t i = 0; i < oamSize; i++){
        if(spriteCollidesCurrentPixel(oamMap[i])){
            sprite = oamMap[i];

            if(sprite.attr0.state.objectMode > 0){
                // affine oder versteckt
                // setPixel(currentCycle, currentScanline, 0, 255, 0);
                continue;
            }
            spriteAlphaOverride = false;
            if(sprite.attr0.state.gfxMode == 1)
                spriteAlphaOverride = true;
            if(sprite.attr0.state.gfxMode == 2)
                continue;
    
            const bool bpp8 = sprite.attr0.state.colorMode;
            const Word tileOffset =  0x20; // egal ob 8bbp oder 4bbp
            int spriteX = sprite.attr1.state.xCoord;
            if(spriteX > 240)
                spriteX -= 512;
            int spriteY = sprite.attr0.state.yCoord;
            if(spriteY > 160)
                spriteY -= 256;
            const auto [sizeX, sizeY] = spriteShapeSizeTable[sprite.attr0.state.spriteShape][sprite.attr1.state.spriteSize];
            Word tileSizeX = sizeX / 8;
            const Word tileSizeY = sizeY / 8;
            const Word xOffset = currentCycle - spriteX;
            const Word yOffset = currentScanline - spriteY;
    
            const Word baseTileIndex = sprite.attr2.state.baseTileIndex;
    
            const Word paletteBank = sprite.attr2.state.paletteBank; // nur in 4bbp modus
    
            Byte* vRamTiles = vRam.data() + 0x00010000;
    
            Word currentTileX = xOffset / 8;
            if(sprite.attr1.state.getHorizontalFlip()){
                currentTileX = tileSizeX - 1 - currentTileX;
            }
            Word currentTileY = yOffset / 8;
            if(sprite.attr1.state.getVerticalFlip()){
                currentTileY = tileSizeY - 1 - currentTileY;
            }
            const Word inTileX = xOffset % 8;
            const Word inTileY = yOffset % 8;
        
            if(!spriteMappingMode1D){
                tileSizeX = 32; // 32 Tiles in Folge, dann kommt die nächste Zeile
            }
        
            const Word tileIndex = baseTileIndex + currentTileX + currentTileY * tileSizeX;
        
            const Word tileWidthByte = bpp8 ? 8 : 4;
        
            Byte* tileStart = vRamTiles + tileOffset * tileIndex;
        
            const Word verticalFactor = sprite.attr1.state.getVerticalFlip() ? -1 : 1;
            const Word horizontalFactor = sprite.attr1.state.getHorizontalFlip() ? -1 : 1;
        
            const Word verticalWidth = sprite.attr1.state.getVerticalFlip() ? 7 * tileWidthByte : 0;
            const Word horizontalWidth = sprite.attr1.state.getHorizontalFlip() ? (bpp8 ? 7 : 3) : 0;
        
            const Word bitWidthInTile = bpp8 ? inTileX : inTileX / 2;
        
            // 8px x 4 bit = 32 bit = 4 byte, bei 8bbp 8 byte
            const Word pixelIndex = (verticalWidth + verticalFactor * inTileY * tileWidthByte) + (horizontalWidth + horizontalFactor * bitWidthInTile);
            if(tileStart + pixelIndex >= vRam.data() + vRam.size()) return result;
            Byte pixel = *(tileStart + pixelIndex);
            if(!bpp8){
                if((inTileX & 1) ^ sprite.attr1.state.getHorizontalFlip()) pixel >>= 4;
                pixel &= 0xF;
            }
            Byte paletteIndex =  bpp8 ? pixel : pixel | (paletteBank << 4);
            HalfWord colorLow = paletteRam[2 * Word(paletteIndex) + 0x200]; // Byte adressiert, palette Bank von Sprites offset um 0x200
            HalfWord colorHigh = paletteRam[2 * Word(paletteIndex) + 0x201];
            HalfWord color = colorLow | (colorHigh << 8);
            Byte red = color & 0b11111;
            Byte green = (color >> 5) & 0b11111;
            Byte blue = (color >> 10) & 0b11111;
            if(pixel > 0){
                insideObjectWindow = true;
                result = {.pixel = pixel, .red = Byte(red << 3), .green = Byte(green << 3), .blue = Byte(blue << 3), .priority = (Byte)sprite.attr2.state.priority};
            }
        }
    }

    return result;
}

// https://www.coranac.com/tonc/text/regbg.htm
// screen entry index für tile-coord paar
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


PIXEL_T PPU::drawBG(const BGCNT_T &CONTROL, const HalfWord &BGX, const HalfWord &BGY){
    PIXEL_T result;
    result.priority = 99;
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
    if(tileStart + pixelIndex >= vRam.data() + vRam.size()) return result;
    Byte pixel = *(tileStart + pixelIndex);
    if(!bpp8){
        if((inTileX & 1) ^ entry.state.flipHorizontal) pixel >>= 4;
        pixel &= 0xF;
    }
    Byte paletteIndex =  bpp8 ? pixel : pixel | (entry.state.paletteBank << 4);
    HalfWord colorLow = paletteRam[2 * Word(paletteIndex)]; // Byte adressiert
    HalfWord colorHigh = paletteRam[2 * Word(paletteIndex) + 1];
    HalfWord color = colorLow | (colorHigh << 8);
    Byte red = color & 0b11111;
    Byte green = (color >> 5) & 0b11111;
    Byte blue = (color >> 10) & 0b11111;
    if(pixel > 0)
        result = {.pixel = pixel, .red = Byte(red << 3), .green = Byte(green << 3), .blue = Byte(blue << 3), .priority = (Byte)CONTROL.state.BGPriority};

    return result;
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


// Pixel Reihenfolge: https://raddad772.github.io/2025/01/02/notes-on-GBA-PPU-windows-and-blending.html
// Ist aktuell noch falsch
void gba::PPU::drawPixelMode0() {
    if(currentCycle == 0){
        this->detectSpritesOnScanline();
    }
    setPixel(currentCycle, currentScanline, 0, 0, 0);
    bgOrderSize = 0;
    for(int i = 3; i >= 0; i--){
        insertBGIntoSorted(bgOrder, i, bgOrderSize);
    }

    WINDOW_ACTIVES_T actives = WINDOW_ACTIVES_T{.enableBG0 = true, .enableBG1 = true, .enableBG2 = true, .enableBG3 = true, .enableObj = true, .enableSpecialFX = true};
    
    if(LCDCONTROL.state.displayWin0 || LCDCONTROL.state.displayWin1 || LCDCONTROL.state.displayObjWindow){
        actives = getActives(2); // Win outside
        if(LCDCONTROL.state.displayObjWindow && insideObjectWindow) actives = getActives(3);
        if(LCDCONTROL.state.displayWin1 && insideWindow1()) actives = getActives(1);
        if(LCDCONTROL.state.displayWin0 && insideWindow0()) actives = getActives(0);
    }

    std::array<PIXEL_T, 4> bgPixels = {{{.priority = 99}, {.priority = 99}, {.priority = 99}, {.priority = 99}}}; // pixel in bg-Reihenfolge
    PIXEL_T objPixel = LCDCONTROL.state.displayOBJ ? this->drawSprites() : PIXEL_T{.priority = 99};
    PIXEL_T output_color;
    output_color.priority = 99;

    for(int i = 0; i < 4; i++){
        if(displayBG(i)){
            bgPixels[i] = drawBG(BG_CNT[i], BG_X_OFFSET[i], BG_Y_OFFSET[i]);
        }
    }
    if(false){//actives.enableSpecialFX && SPECIAL_EFFECTS.state.specialEffect > 0){
        output_color = {.pixel = 1, .red = 255, .green = 0, .blue = 0, .priority = 0};
    }
    else{
        for(int i = 0; i < 4; i++){
            int bg = bgOrder[i];
            if(displayBG(bg) && actives.bgActive(bg) && bgPixels[bg].priority < 99)
                output_color = bgPixels[bg];
        }
        if(objPixel.priority <= output_color.priority && actives.enableObj)
            output_color = objPixel;
    }

    setPixel(currentCycle, currentScanline, output_color.red, output_color.green, output_color.blue);
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
