#include "ppu.h"
#include "framework/stringlib.h"
#include <iostream>

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