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

template<typename T, size_t N> // Insertion Sort für EIN Element in bereits sortiertes Array
void insertIntoSorted(std::array<T, N> &sorted, const T &element, size_t &oldSize){
    sorted[oldSize] = element;
    size_t i = oldSize;
    oldSize++;
    while(i > 0 && sorted[i] < sorted[i-1]){
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

WINDOW_ACTIVES_T gba::PPU::getActives()
{
    WINDOW_ACTIVES_T actives = WINDOW_ACTIVES_T{.enableBG0 = true, .enableBG1 = true, .enableBG2 = true, .enableBG3 = true, .enableObj = true, .enableSpecialFX = true};
    
    if(LCDCONTROL.state.displayWin0 || LCDCONTROL.state.displayWin1 || LCDCONTROL.state.displayObjWindow){
        actives = getActives(2); // Win outside
        if(LCDCONTROL.state.displayObjWindow && insideObjectWindow) actives = getActives(3);
        if(LCDCONTROL.state.displayWin1 && insideWindow1()) actives = getActives(1);
        if(LCDCONTROL.state.displayWin0 && insideWindow0()) actives = getActives(0);
    }

    return actives;
}

bool PPU::insideWindow0(){
    if(WINDOW_0_H.state.leftMost > WINDOW_0_H.state.rightMostPlus1
        && (currentCycle < WINDOW_0_H.state.rightMostPlus1 || currentCycle >= WINDOW_0_H.state.leftMost)
        && currentScanline >= WINDOW_0_V.state.topMost && currentScanline < WINDOW_0_V.state.bottomMostPlus1
    ){
        return true;
    }
    return currentCycle >= WINDOW_0_H.state.leftMost && currentCycle < WINDOW_0_H.state.rightMostPlus1
    && currentScanline >= WINDOW_0_V.state.topMost && currentScanline < WINDOW_0_V.state.bottomMostPlus1;
}

bool PPU::insideWindow1(){
    if(WINDOW_1_H.state.leftMost > WINDOW_1_H.state.rightMostPlus1
        && (currentCycle < WINDOW_1_H.state.rightMostPlus1 || currentCycle >= WINDOW_1_H.state.leftMost)
        && currentScanline >= WINDOW_1_V.state.topMost && currentScanline < WINDOW_1_V.state.bottomMostPlus1
    ){
        return true;
    }
    return currentCycle >= WINDOW_1_H.state.leftMost && currentCycle < WINDOW_1_H.state.rightMostPlus1
    && currentScanline >= WINDOW_1_V.state.topMost && currentScanline < WINDOW_1_V.state.bottomMostPlus1;
}

bool PPU::hasTargetA(int index){
    switch(index){
        case 0:
            return SPECIAL_EFFECTS.state.targetA_obj;
        case 1:
            return SPECIAL_EFFECTS.state.targetA_bg0;
        case 2:
            return SPECIAL_EFFECTS.state.targetA_bg1;
        case 3:
            return SPECIAL_EFFECTS.state.targetA_bg2;
        case 4:
            return SPECIAL_EFFECTS.state.targetA_bg3;
        case 5:
            return SPECIAL_EFFECTS.state.targetA_bd;
        default:
            return false;
    }
}

bool PPU::hasTargetB(int index){
    switch(index){
        case 0:
            return SPECIAL_EFFECTS.state.targetB_obj;
        case 1:
            return SPECIAL_EFFECTS.state.targetB_bg0;
        case 2:
            return SPECIAL_EFFECTS.state.targetB_bg1;
        case 3:
            return SPECIAL_EFFECTS.state.targetB_bg2;
        case 4:
            return SPECIAL_EFFECTS.state.targetB_bg3;
        case 5:
            return SPECIAL_EFFECTS.state.targetB_bd;
        default:
            return false;
    }
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
        if(currentScanline < 160){
            // dmx, dmy increment affine bg
            for(int i = 0; i < 2; i++){
                BG_REFERENCE_X[i] += sign_extend_n_32(BG_PB[i], 16);
                BG_REFERENCE_Y[i] += sign_extend_n_32(BG_PD[i], 16);

                BG_REFERENCE_LINE_X[i] = BG_REFERENCE_X[i];
                BG_REFERENCE_LINE_Y[i] = BG_REFERENCE_Y[i];
            }

            if(LCDSTATUS.state.hBlankIE){
                bus.lock()->setIF(1, true);
            }
        }
        bus.lock()->PPUEnteredHBlank();
    }
    else if(currentCycle >= 1232){
        currentCycle = 0;
        LCDSTATUS.state.hBlankFlag = 0;
        currentScanline++;
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
        for(int i = 0; i < 2; i++){
            // 20.8 bit signed
            Word refX = 0x0FFFFFFF & ((Word(BG_DXH[i]) << 16) | BG_DXL[i]);
            Word refY = 0x0FFFFFFF & ((Word(BG_DYH[i]) << 16) | BG_DYL[i]);
            
            BG_REFERENCE_X[i] = sign_extend_n_32(refX, 28);
            BG_REFERENCE_Y[i] = sign_extend_n_32(refY, 28);

            BG_REFERENCE_LINE_X[i] = BG_REFERENCE_X[i];
            BG_REFERENCE_LINE_Y[i] = BG_REFERENCE_Y[i];
        }
        bus.lock()->PPUEnteredVBlank();
    }
    else if(currentScanline >= 228){
        currentScanline = 0;
        LCDSTATUS.state.vCounterFlag = 0;
    }
    if(currentCycle == 0 && currentScanline == LCDSTATUS.state.vCountSetting && LCDSTATUS.state.vCounterIE){
        bus.lock()->setIF(2, true);
        LCDSTATUS.state.vCounterFlag = 1;
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

        // Affine bg matrix
        for(int i = 0; i < 2; i++){
            BG_REFERENCE_LINE_X[i] += sign_extend_n_32(BG_PA[i], 16);
            BG_REFERENCE_LINE_Y[i] += sign_extend_n_32(BG_PC[i], 16);
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

std::pair<size_t, size_t> getCenter(int x, int y, int sizeX, int sizeY){
    return std::pair<size_t, size_t>{x + (sizeX / 2), y + (sizeY / 2)};
}

void PPU::detectSpritesOnScanline(){
    this->oamAttribsCurrentLineSize = 0;
    const int y = currentScanline;

    // sicher?
    OAMAttribs* oamMap = (OAMAttribs*)this->oamAttribs.data();
    // Rückwärtsiteration fixt die Sprite Priorität, nochmal anschauen
    constexpr int oamSize = 128;
    for(int i = oamSize-1; i >= 0; i--){
        OAMAttribs current = oamMap[i];
        auto [sizeX, sizeY] = spriteShapeSizeTable[current.attr0.state.spriteShape][current.attr1.state.spriteSize];
        (void)sizeX;
        int yStart = current.attr0.state.yCoord;
        int extraSize = 0;
        if(current.attr0.state.objectMode == 3){ // affine doppelt so groß
            yStart -= sizeY;
            extraSize = sizeY;
        }
        const int yEnd = current.attr0.state.yCoord + sizeY + extraSize;
        if((yStart <= y && yEnd > y) || (yStart > 160 && (yEnd - 256) > y)){
            //push
            insertIntoSorted(oamAttribsCurrentLine, current, oamAttribsCurrentLineSize);
        }
    }
}

bool PPU::spriteCollidesCurrentPixel(const OAMAttribs &current){
    const int x = currentCycle;
    const int y = currentScanline;

    auto [sizeX, sizeY] = spriteShapeSizeTable[current.attr0.state.spriteShape][current.attr1.state.spriteSize];
    int xStart = current.attr1.state.xCoord;
    int yStart = current.attr0.state.yCoord;
    int extraX = 0;
    int extraY = 0;
    if(current.attr0.state.objectMode == 3){ // affine doppelt so groß
        xStart -= sizeX;
        extraX = sizeX;

        yStart -= sizeY;
        extraY = sizeY;
    }
    const int xEnd = current.attr1.state.xCoord + sizeX + extraX;
    const int yEnd = current.attr0.state.yCoord + sizeY + extraY;
    return
        ((yStart <= y && yEnd > y) || (yStart > 160 && (yEnd - 256) > y))
        && ((xStart <= x && xEnd > x) || (xStart > 240 && (xEnd - 512) > x));
}

PIXEL_T PPU::drawSprites(){
    PIXEL_T result;
    result.priority = 99;
    result.layerIndex = 0;
    bool spriteMappingMode1D = LCDCONTROL.state.ObjCharVRAMMapping;
    insideObjectWindow = false;
    spriteAlphaOverride = false;


    const OAMAttribs* oamMap = this->oamAttribsCurrentLine.data();
    const size_t oamSize = oamAttribsCurrentLineSize;

    for(size_t i = 0; i < oamSize; i++){
        if(spriteCollidesCurrentPixel(oamMap[i])){
            bool currentSpriteHasOverride = false;
            const OAMAttribs sprite = oamMap[i];

            if(sprite.attr0.state.objectMode == 2){
                // versteckt
                continue;
            }
            if(sprite.attr0.state.gfxMode == 1)
                currentSpriteHasOverride = true;
            else if(sprite.attr0.state.gfxMode == 0)
                currentSpriteHasOverride = false;

            if(sprite.attr0.state.gfxMode == 2)
                continue;
    
            const bool bpp8 = sprite.attr0.state.colorMode;
            const Word tileOffset =  bpp8 ? 0x40 : 0x20;
            int spriteX = sprite.attr1.state.xCoord;
            if(spriteX > 240)
                spriteX -= 512;
            int spriteY = sprite.attr0.state.yCoord;
            if(spriteY > 160)
                spriteY -= 256;
            auto [sizeX, sizeY] = spriteShapeSizeTable[sprite.attr0.state.spriteShape][sprite.attr1.state.spriteSize];
            const Word sizeXHalf = sizeX / 2 - 1;
            const Word sizeYHalf = sizeY / 2 - 1;

            Word tileSizeX = sizeX / 8;
            const Word tileSizeY = sizeY / 8;
            Word xOffset;
            Word yOffset;

            // Affine sprite oder 2x affine sprite
            if(sprite.attr0.state.objectMode == 1 || sprite.attr0.state.objectMode == 3){
                auto [centerX, centerY] = getCenter(spriteX, spriteY, sizeX, sizeY);
                if(sprite.attr0.state.objectMode == 3){
                    centerX += sizeXHalf;
                    centerY += sizeYHalf;
                }
                const Word affineIndex = sprite.attr1.state.affineIndex;
                const AffineAttribs affineMatrix = *(((AffineAttribs*)this->oamAttribs.data()) + affineIndex);

                xOffset = sizeXHalf + ((int(centerX - currentCycle) * affineMatrix.pa + int(centerY - currentScanline) * affineMatrix.pb) >> 8);
                yOffset = sizeYHalf + ((int(centerX - currentCycle) * affineMatrix.pc + int(centerY - currentScanline) * affineMatrix.pd) >> 8);
                
                // Ist aus irgendeinem Grund gespiegelt
                xOffset = sizeX - xOffset - 1;
                yOffset = sizeY - yOffset - 1;
            }
            else{
                xOffset = currentCycle - spriteX;
                yOffset = currentScanline - spriteY;
            }

            // Außerhalb von Sprite, wichtig bei affinen
            if(xOffset >= sizeX || yOffset >= sizeY)
                continue;
    
            const Word baseTileIndex = sprite.attr2.state.baseTileIndex;
    
            const Word paletteBank = sprite.attr2.state.paletteBank; // nur in 4bbp modus
    
            Byte* vRamTiles = vRam.data() + 0x00010000;
    
            Word currentTileX = xOffset / 8;
            Word currentTileY = yOffset / 8;

            // Nur nicht affine Sprites!
            const bool flipH = sprite.attr0.state.objectMode == 0 && sprite.attr1.state.getHorizontalFlip();
            const bool flipV = sprite.attr0.state.objectMode == 0 && sprite.attr1.state.getVerticalFlip();

            if(flipH){
                currentTileX = tileSizeX - 1 - currentTileX;
            }
            if(flipV){
                currentTileY = tileSizeY - 1 - currentTileY;
            }
            const Word inTileX = xOffset % 8;
            const Word inTileY = yOffset % 8;
        
            if(!spriteMappingMode1D){
                tileSizeX = 32; // 32 Tiles in Folge, dann kommt die nächste Zeile
            }
        
            const Word tileIndex = currentTileX + currentTileY * tileSizeX;
        
            const Word tileWidthByte = bpp8 ? 8 : 4;
        
            Byte* tileStart = vRamTiles + 0x20 * baseTileIndex + tileOffset * tileIndex;
        
            const Word verticalFactor = flipV ? -1 : 1;
            const Word horizontalFactor = flipH ? -1 : 1;
        
            const Word verticalWidth = flipV ? 7 * tileWidthByte : 0;
            const Word horizontalWidth = flipH ? (bpp8 ? 7 : 3) : 0;
        
            const Word bitWidthInTile = bpp8 ? inTileX : inTileX / 2;
        
            // 8px x 4 bit = 32 bit = 4 byte, bei 8bbp 8 byte
            const Word pixelIndex = (verticalWidth + verticalFactor * inTileY * tileWidthByte) + (horizontalWidth + horizontalFactor * bitWidthInTile);
            if(tileStart + pixelIndex >= vRam.data() + vRam.size())
                continue;
            Byte pixel = *(tileStart + pixelIndex);
            if(!bpp8){
                if((inTileX & 1) ^ flipH) pixel >>= 4;
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
                if(sprite.attr2.state.priority <= result.priority){
                    result = {.pixel = pixel, .red = Byte(red << 3), .green = Byte(green << 3), .blue = Byte(blue << 3), .priority = (Byte)sprite.attr2.state.priority, .layerIndex = 0};
                    spriteAlphaOverride = currentSpriteHasOverride;
                }
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
constexpr std::array<std::pair<Word, Word>, 4> affineBgrSizes =  {std::pair{128,128}, std::pair{256,256}, std::pair{512,512}, std::pair{1024,1024}};

template <bool isAffine>
PIXEL_T PPU::drawBG(const int index)
{
    const BGCNT_T &CONTROL = BG_CNT[index];
    const int affineIndex = index - 2;
    const bool affineWrapping = CONTROL.state.DisplayAreaOverflowBG2BG3;

    PIXEL_T result;
    result.priority = 99;
    result.layerIndex = index + 1;
    const int pixelX = currentCycle;
    const int pixelY = currentScanline;
//    Memory	0600:0000	0600:4000	0600:8000	0600:C000
// charblock        0	        1	        2	        3
// screenblock	0	…	7	8	…	15	16	…	23	24	…	31

    const Word screenblockSize = 0x800;
    const Word chrblockSize = 0x4000;
    const Word screenBaseBlock = CONTROL.state.screenBaseBlock;
    const Word characterBaseBlock = CONTROL.state.CHRBaseBlock;
    auto [bgrX, bgrY] = isAffine ? affineBgrSizes[CONTROL.state.screenSize] : regularBgrSizes[CONTROL.state.screenSize];
    Byte* entries = vRam.data() + (screenBaseBlock * screenblockSize);
    Byte* chrEntries = vRam.data() + (characterBaseBlock * chrblockSize);
    bool bpp8 = isAffine ? true : CONTROL.state.colorsPalettes;
    const Word tileWidth = bpp8 ? 0x40 : 0x20;
    const Word tileWidthByte = bpp8 ? 8 : 4;
    Word mapX;
    Word mapY;

    if(isAffine){

        mapX = BG_REFERENCE_LINE_X[affineIndex] >> 8;
        mapY = BG_REFERENCE_LINE_Y[affineIndex] >> 8;

        if(affineWrapping){
            mapX %= bgrX;
            mapY %= bgrY;
        }
        else if(mapX < 0 || mapX >= bgrX || mapY < 0 || mapY >= bgrY){
            return result;
        }

    }
    else{
        mapX = pixelX + (this->BG_X_OFFSET[index] & 0x1FF);
        mapY = pixelY + (this->BG_Y_OFFSET[index] & 0x1FF);
        mapX %= bgrX;
        mapY %= bgrY;
    }

    Word tileCoordX = mapX / 8;
    Word tileCoordY = mapY / 8;

    Word sbb = isAffine ? tileCoordX + tileCoordY * (bgrX / 8) : seIndexFast(tileCoordX, tileCoordY, CONTROL);
    Word inTileX = mapX % 8;
    Word inTileY = mapY % 8;
    Word screenIndex = sbb;
    const Word screenEntrySize = isAffine ? 1 : 2;
    Byte entryLow = entries[screenIndex * screenEntrySize];
    Byte entryHigh = isAffine ? 0 : entries[screenIndex * screenEntrySize + 1];
    ScreenEntry entry;
    entry.raw = entryLow | (HalfWord(entryHigh) << 8);
    Word tileID = isAffine ? entryLow : entry.state.TileID;
    Byte* tileStart = chrEntries + tileWidth * tileID;
    // Flipping:
    bool flipH = !isAffine && entry.state.flipHorizontal;
    bool flipV = !isAffine && entry.state.flipVertical;

    const Word verticalFactor =     flipV ? -1 : 1;
    const Word horizontalFactor =   flipH ? -1 : 1;

    const Word verticalWidth = flipV ? 7 * tileWidthByte : 0;
    const Word horizontalWidth = flipH ? (bpp8 ? 7 : 3) : 0;

    const Word bitWidthInTile = bpp8 ? inTileX : inTileX / 2;

    // 8px x 4 bit = 32 bit = 4 byte, bei 8bbp 8 byte
    const Word pixelIndex = (verticalWidth + verticalFactor * inTileY * tileWidthByte) + (horizontalWidth + horizontalFactor * bitWidthInTile);
    if(tileStart + pixelIndex >= vRam.data() + vRam.size()) return result;
    Byte pixel = *(tileStart + pixelIndex);
    if(!bpp8){
        if((inTileX & 1) ^ flipH) pixel >>= 4;
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
        result = {.pixel = pixel, .red = Byte(red << 3), .green = Byte(green << 3), .blue = Byte(blue << 3), .priority = (Byte)CONTROL.state.BGPriority, .layerIndex = Byte(index + 1)};

    return result;
}

PIXEL_T PPU::getBackdrop(){
    HalfWord colorLow = paletteRam[0];
    HalfWord colorHigh = paletteRam[1];
    HalfWord color = colorLow | (colorHigh << 8);
    Byte red = color & 0b11111;
    Byte green = (color >> 5) & 0b11111;
    Byte blue = (color >> 10) & 0b11111;
    return PIXEL_T{
        .pixel = 0,
        .red = Byte(red << 3),
        .green = Byte(green << 3),
        .blue = Byte(blue << 3),
        .priority = 9,
        .layerIndex = 5
    };
}

PIXEL_T PPU::blend(PIXEL_T &p1, PIXEL_T &p2){
    Word coefA = std::min((unsigned int)ALPHA_BLENDING.state.coefA, 16u);
    Word coefB = std::min((unsigned int)ALPHA_BLENDING.state.coefB, 16u);
    PIXEL_T res = p1;
    p1.red >>= 3; p1.green >>= 3; p1.blue >>= 3;
    p2.red >>= 3; p2.green >>= 3; p2.blue >>= 3;
    Word red = (Word(p1.red) * coefA + Word(p2.red) * coefB + 8u) >> 4;
    Word green = (Word(p1.green) * coefA + Word(p2.green) * coefB + 8u) >> 4;
    Word blue = (Word(p1.blue) * coefA + Word(p2.blue) * coefB + 8u) >> 4;
    res.red = std::min(red << 3, 255u);
    res.green = std::min(green << 3, 255u);
    res.blue = std::min(blue << 3, 255u);
    return res;
}

PIXEL_T PPU::brighten(const PIXEL_T &p){
    Word coef = std::min((unsigned int)BRIGHTNESS_FADE.state.coef, 16u);
    PIXEL_T res = p;
    res.red >>= 3; res.green >>= 3; res.blue >>= 3;
    res.red += ((31 - res.red) * coef + 8) / 16;
    res.green += ((31 - res.green) * coef + 8) / 16;
    res.blue += ((31 - res.blue) * coef + 8) / 16;
    res.red <<= 3; res.green <<= 3; res.blue <<= 3;
    return res;
}

PIXEL_T PPU::darken(const PIXEL_T &p){
    Word coef = std::min((unsigned int)BRIGHTNESS_FADE.state.coef, 16u);
    PIXEL_T res = p;
    res.red >>= 3; res.green >>= 3; res.blue >>= 3;
    res.red -= (res.red * coef + 7) / 16;
    res.green -= (res.green * coef + 7) / 16;
    res.blue -= (res.blue * coef + 7) / 16;
    res.red <<= 3; res.green <<= 3; res.blue <<= 3;
    return res;
}

void gba::PPU::setColorFromLayerOrder(const WINDOW_ACTIVES_T &actives)
{
    PIXEL_T output_color{.priority = 99};
    PIXEL_T target_a{.priority = 99};
    PIXEL_T target_b{.priority = 99};

    output_color = layerOrder[layerOrderSize - 1];

    if(layerOrderSize > 1){
        target_a = layerOrder[layerOrderSize - 1];
        target_b = layerOrder[layerOrderSize - 2];
        output_color = target_a;

        mixFinalColor(actives, target_a, target_b, output_color);
    }

    setPixel(currentCycle, currentScanline, output_color.red, output_color.green, output_color.blue);
}

void PPU::mixFinalColor(const WINDOW_ACTIVES_T &actives, PIXEL_T &targetA, PIXEL_T &targetB, PIXEL_T &output)
{
    // target a ist transparenter sprite
    if(actives.enableSpecialFX || (spriteAlphaOverride && targetA.layerIndex == 0 && hasTargetB(targetB.layerIndex))){
        if(spriteAlphaOverride && targetA.layerIndex == 0 && hasTargetB(targetB.layerIndex)){
            output = blend(targetA, targetB);
        }
        else if(SPECIAL_EFFECTS.state.specialEffect == 1 && hasTargetA(targetA.layerIndex) && hasTargetB(targetB.layerIndex)){
            output = blend(targetA, targetB);
        }
        else if(SPECIAL_EFFECTS.state.specialEffect == 2 && hasTargetA(targetA.layerIndex)){
            output = brighten(targetA);
        }
        else if(SPECIAL_EFFECTS.state.specialEffect == 3 && hasTargetA(targetA.layerIndex)){
            output = darken(targetA);
        }
    }
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
void gba::PPU::drawPixelMode0() {
    layerOrderSize = 0;
    if(currentCycle == 0){
        this->detectSpritesOnScanline();
    }

    WINDOW_ACTIVES_T actives = getActives();

    insertIntoSorted(layerOrder, getBackdrop(), layerOrderSize); // niedrigste Prio
    for(int i = 3; i >= 0; i--){
        if(displayBG(i) && actives.bgActive(i)){
            PIXEL_T bg = drawBG<false>(i);
            if(bg.priority < 99)
                insertIntoSorted(layerOrder, bg, layerOrderSize);
        }
    }
    if(actives.enableObj && LCDCONTROL.state.displayOBJ){
        insertIntoSorted(layerOrder, this->drawSprites(), layerOrderSize); // höchste prio
    }

    setColorFromLayerOrder(actives);
}

void gba::PPU::drawPixelMode1() {
    layerOrderSize = 0;
    if(currentCycle == 0){
        this->detectSpritesOnScanline();
    }

    WINDOW_ACTIVES_T actives = getActives();

    insertIntoSorted(layerOrder, getBackdrop(), layerOrderSize); // niedrigste Prio

    //affine
    if(displayBG(2) && actives.bgActive(2)){
        PIXEL_T bg = drawBG<true>(2);
        if(bg.priority < 99)
            insertIntoSorted(layerOrder, bg, layerOrderSize);
    }

    for(int i = 1; i >= 0; i--){
        if(displayBG(i) && actives.bgActive(i)){
            PIXEL_T bg = drawBG<false>(i);
            if(bg.priority < 99)
                insertIntoSorted(layerOrder, bg, layerOrderSize);
        }
    }
    if(actives.enableObj && LCDCONTROL.state.displayOBJ){
        insertIntoSorted(layerOrder, this->drawSprites(), layerOrderSize); // höchste prio
    }

    setColorFromLayerOrder(actives);
}

void gba::PPU::drawPixelMode2() {
    layerOrderSize = 0;
    if(currentCycle == 0){
        this->detectSpritesOnScanline();
    }

    WINDOW_ACTIVES_T actives = getActives();

    insertIntoSorted(layerOrder, getBackdrop(), layerOrderSize); // niedrigste Prio

    //affine
    if(displayBG(3) && actives.bgActive(3)){
        PIXEL_T bg = drawBG<true>(3);
        if(bg.priority < 99)
            insertIntoSorted(layerOrder, bg, layerOrderSize);
    }

    if(displayBG(2) && actives.bgActive(2)){
        PIXEL_T bg = drawBG<true>(2);
        if(bg.priority < 99)
            insertIntoSorted(layerOrder, bg, layerOrderSize);
    }

    if(actives.enableObj && LCDCONTROL.state.displayOBJ){
        insertIntoSorted(layerOrder, this->drawSprites(), layerOrderSize); // höchste prio
    }

    setColorFromLayerOrder(actives);
}

void gba::PPU::drawPixelMode3() {
    layerOrderSize = 0;
    if(currentCycle == 0){
        this->detectSpritesOnScanline();
    }
    int index = currentCycle + 240 * currentScanline;
    HalfWord pixel = HalfWord(vRam[2 * index]) | (HalfWord(vRam[2 * index + 1]) << 8);
    HalfWord red = pixel & 0b11111;
    HalfWord green = (pixel >> 5) & 0b11111;
    HalfWord blue = (pixel >> 10) & 0b11111;
    PIXEL_T bg2 = {.pixel = 1, .red = Byte(red << 3), .green = Byte(green << 3), .blue = Byte(blue << 3), .priority = (Byte)BG_CNT[2].state.BGPriority, .layerIndex = 3};

    WINDOW_ACTIVES_T actives = getActives();

    insertIntoSorted(layerOrder, getBackdrop(), layerOrderSize);

    if(LCDCONTROL.state.displayBG2 && actives.bgActive(2))
        insertIntoSorted(layerOrder, bg2, layerOrderSize);
        
    if(actives.enableObj && LCDCONTROL.state.displayOBJ)
        insertIntoSorted(layerOrder, drawSprites(), layerOrderSize);
    
    setColorFromLayerOrder(actives);
}

void gba::PPU::drawPixelMode4() {
    layerOrderSize = 0;
    if(currentCycle == 0){
        this->detectSpritesOnScanline();
    }
    int index = currentCycle + 240 * currentScanline;
    size_t page = LCDCONTROL.state.frameSelect ? 0xA000 : 0;
    Byte paletteIndex = vRam[index + page];
    HalfWord pixel = HalfWord(paletteRam[2 * paletteIndex]) | (HalfWord(paletteRam[2 * paletteIndex + 1]) << 8);
    HalfWord red = pixel & 0b11111;
    HalfWord green = (pixel >> 5) & 0b11111;
    HalfWord blue = (pixel >> 10) & 0b11111;
    PIXEL_T bg2 = {.pixel = paletteIndex, .red = Byte(red << 3), .green = Byte(green << 3), .blue = Byte(blue << 3), .priority = (Byte)BG_CNT[2].state.BGPriority, .layerIndex = 3};

    WINDOW_ACTIVES_T actives = getActives();

    insertIntoSorted(layerOrder, getBackdrop(), layerOrderSize);

    if(LCDCONTROL.state.displayBG2 && actives.bgActive(2))
        insertIntoSorted(layerOrder, bg2, layerOrderSize);
        
    if(actives.enableObj && LCDCONTROL.state.displayOBJ)
        insertIntoSorted(layerOrder, drawSprites(), layerOrderSize);
    
    setColorFromLayerOrder(actives);
}

void gba::PPU::drawPixelMode5() {
    layerOrderSize = 0;
    if(currentCycle == 0){
        this->detectSpritesOnScanline();
    }
    PIXEL_T bg2;
    if(currentCycle < 160 && currentScanline < 128){
        int index = currentCycle + 160 * currentScanline;
        size_t page = LCDCONTROL.state.frameSelect ? 0xA000 : 0;
        HalfWord pixel = HalfWord(vRam[page + 2 * index]) | (HalfWord(vRam[page + 2 * index + 1]) << 8);
        HalfWord red = pixel & 0b11111;
        HalfWord green = (pixel >> 5) & 0b11111;
        HalfWord blue = (pixel >> 10) & 0b11111;
        bg2 = {.pixel = 1, .red = Byte(red << 3), .green = Byte(green << 3), .blue = Byte(blue << 3), .priority = (Byte)BG_CNT[2].state.BGPriority, .layerIndex = 3};
    }
    else{
        bg2 = PIXEL_T{.pixel = 0, .priority = 99};
    }
    WINDOW_ACTIVES_T actives = getActives();

    insertIntoSorted(layerOrder, getBackdrop(), layerOrderSize);

    if(LCDCONTROL.state.displayBG2 && actives.bgActive(2))
        insertIntoSorted(layerOrder, bg2, layerOrderSize);
        
    if(actives.enableObj && LCDCONTROL.state.displayOBJ)
        insertIntoSorted(layerOrder, drawSprites(), layerOrderSize);
    
    setColorFromLayerOrder(actives);
}
