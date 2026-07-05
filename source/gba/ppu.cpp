#include "ppu.h"
#include "bus.h"
#include <iostream>

uint32_t *gba::PPU::accessFramebuffer()
{
    return framebuffer.data();
}

void gba::PPU::clock() {
    currentCycle++;
    if(currentCycle == 1004){
        // Hblank
        LCDSTATUS.state.hBlankFlag = 1;
        if(LCDSTATUS.state.hBlankIE){
            bus.lock()->setIF(1, true);
        }
    }
    else if(currentCycle >= 1232){
        currentCycle = 0;
        currentScanline++;
        LCDSTATUS.state.hBlankFlag = 0;
        if(currentScanline == LCDSTATUS.state.vCountSetting && LCDSTATUS.state.vCounterIE){
            bus.lock()->setIF(2, true);
            LCDSTATUS.state.vCounterFlag = 1;
        }
    }
    if(currentScanline == 160 && currentCycle == 0){
        // Vblank
        LCDSTATUS.state.vBlankFlag = 1;
        if(LCDSTATUS.state.vBlankIE){
            // std::cout << "Vblank IRQ" << std::endl;
            bus.lock()->setIF(0, true);
        }
        hasframe = true;
    }
    else if(currentScanline >= 228){
        currentScanline = 0;
        LCDSTATUS.state.vCounterFlag = 0;
        LCDSTATUS.state.vBlankFlag = 0;
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
    setPixel(currentCycle, currentScanline, 255, 0, 0);
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
    switch(addr){
        case 0x04000000:
            LCDCONTROL.raw = (LCDCONTROL.raw & 0xFF00) | val;
            LCDCONTROL.state.cgbMode = 0;
            break;
        case 0x04000001:
            LCDCONTROL.raw = (LCDCONTROL.raw & 0x00FF) | (HalfWord(val) << 8);
            break;
        case 0x04000004:
            LCDSTATUS.raw = (LCDSTATUS.raw & 0xFF00) | val;
            break;
        case 0x04000005:
            LCDSTATUS.raw = (LCDSTATUS.raw & 0x00FF) | (HalfWord(val) << 8);
            break;
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
    }
    return 0;
}

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
    else return 0;
}
