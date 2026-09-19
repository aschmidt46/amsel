#include "ppu.h"
#include "framework/stringlib.h"
#include <iostream>
#include "../arm/arm7tdmi.h"

void gba::PPU::updateAffineScroll(int32_t &Reference, HalfWord low, HalfWord high)
{
    Word ref = 0x0FFFFFFF & ((Word(high) << 16) | low); 
    Reference = sign_extend_n_32(ref, 28);
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

    else if(addr == 0x04000020){
        BG_PA[0] = (BG_PA[0] & 0xFF00) | val;
    }
    else if(addr == 0x04000021){
        BG_PA[0] = (BG_PA[0] & 0xFF) | (HalfWord(val) << 8);
    }
    else if(addr == 0x04000022){
        BG_PB[0] = (BG_PB[0] & 0xFF00) | val;
    }
    else if(addr == 0x04000023){
        BG_PB[0] = (BG_PB[0] & 0xFF) | (HalfWord(val) << 8);
    }
    else if(addr == 0x04000024){
        BG_PC[0] = (BG_PC[0] & 0xFF00) | val;
    }
    else if(addr == 0x04000025){
        BG_PC[0] = (BG_PC[0] & 0xFF) | (HalfWord(val) << 8);
    }
    else if(addr == 0x04000026){
        BG_PD[0] = (BG_PD[0] & 0xFF00) | val;
    }
    else if(addr == 0x04000027){
        BG_PD[0] = (BG_PD[0] & 0xFF) | (HalfWord(val) << 8);
    }

    else if(addr == 0x04000028){
        BG_DXL[0] = (BG_DXL[0] & 0xFF00) | val;
        updateAffineScroll(BG_REFERENCE_X[0], BG_DXL[0], BG_DXH[0]);
    }
    else if(addr == 0x04000029){
        BG_DXL[0] = (BG_DXL[0] & 0xFF) | (HalfWord(val) << 8);
        updateAffineScroll(BG_REFERENCE_X[0], BG_DXL[0], BG_DXH[0]);
    }
    else if(addr == 0x0400002A){
        BG_DXH[0] = (BG_DXH[0] & 0xFF00) | val;
        updateAffineScroll(BG_REFERENCE_X[0], BG_DXL[0], BG_DXH[0]);
    }
    else if(addr == 0x0400002B){
        BG_DXH[0] = (BG_DXH[0] & 0xFF) | (HalfWord(val) << 8);
        updateAffineScroll(BG_REFERENCE_X[0], BG_DXL[0], BG_DXH[0]);
    }

    else if(addr == 0x0400002C){
        BG_DYL[0] = (BG_DYL[0] & 0xFF00) | val;
        updateAffineScroll(BG_REFERENCE_Y[0], BG_DYL[0], BG_DYH[0]);
    }
    else if(addr == 0x0400002D){
        BG_DYL[0] = (BG_DYL[0] & 0xFF) | (HalfWord(val) << 8);
        updateAffineScroll(BG_REFERENCE_Y[0], BG_DYL[0], BG_DYH[0]);
    }
    else if(addr == 0x0400002E){
        BG_DYH[0] = (BG_DYH[0] & 0xFF00) | val;
        updateAffineScroll(BG_REFERENCE_Y[0], BG_DYL[0], BG_DYH[0]);
    }
    else if(addr == 0x0400002F){
        BG_DYH[0] = (BG_DYH[0] & 0xFF) | (HalfWord(val) << 8);
        updateAffineScroll(BG_REFERENCE_Y[0], BG_DYL[0], BG_DYH[0]);
    }

    else if(addr == 0x04000030){
        BG_PA[1] = (BG_PA[1] & 0xFF00) | val;
    }
    else if(addr == 0x04000031){
        BG_PA[1] = (BG_PA[1] & 0xFF) | (HalfWord(val) << 8);
    }
    else if(addr == 0x04000032){
        BG_PB[1] = (BG_PB[1] & 0xFF00) | val;
    }
    else if(addr == 0x04000033){
        BG_PB[1] = (BG_PB[1] & 0xFF) | (HalfWord(val) << 8);
    }
    else if(addr == 0x04000034){
        BG_PC[1] = (BG_PC[1] & 0xFF00) | val;
    }
    else if(addr == 0x04000035){
        BG_PC[1] = (BG_PC[1] & 0xFF) | (HalfWord(val) << 8);
    }
    else if(addr == 0x04000036){
        BG_PD[1] = (BG_PD[1] & 0xFF00) | val;
    }
    else if(addr == 0x04000037){
        BG_PD[1] = (BG_PD[1] & 0xFF) | (HalfWord(val) << 8);
    }

    else if(addr == 0x04000038){
        BG_DXL[1] = (BG_DXL[1] & 0xFF00) | val;
        updateAffineScroll(BG_REFERENCE_X[1], BG_DXL[1], BG_DXH[1]);
    }
    else if(addr == 0x04000039){
        BG_DXL[1] = (BG_DXL[1] & 0xFF) | (HalfWord(val) << 8);
        updateAffineScroll(BG_REFERENCE_X[1], BG_DXL[1], BG_DXH[1]);
    }
    else if(addr == 0x0400003A){
        BG_DXH[1] = (BG_DXH[1] & 0xFF00) | val;
        updateAffineScroll(BG_REFERENCE_X[1], BG_DXL[1], BG_DXH[1]);
    }
    else if(addr == 0x0400003B){
        BG_DXH[1] = (BG_DXH[1] & 0xFF) | (HalfWord(val) << 8);
        updateAffineScroll(BG_REFERENCE_X[1], BG_DXL[1], BG_DXH[1]);
    }

    else if(addr == 0x0400003C){
        BG_DYL[1] = (BG_DYL[1] & 0xFF00) | val;
        updateAffineScroll(BG_REFERENCE_Y[1], BG_DYL[1], BG_DYH[1]);
    }
    else if(addr == 0x0400003D){
        BG_DYL[1] = (BG_DYL[1] & 0xFF) | (HalfWord(val) << 8);
        updateAffineScroll(BG_REFERENCE_Y[1], BG_DYL[1], BG_DYH[1]);
    }
    else if(addr == 0x0400003E){
        BG_DYH[1] = (BG_DYH[1] & 0xFF00) | val;
        updateAffineScroll(BG_REFERENCE_Y[1], BG_DYL[1], BG_DYH[1]);
    }
    else if(addr == 0x0400003F){
        BG_DYH[1] = (BG_DYH[1] & 0xFF) | (HalfWord(val) << 8);
        updateAffineScroll(BG_REFERENCE_Y[1], BG_DYL[1], BG_DYH[1]);
    }



    else if(addr == 0x04000040){
        WINDOW_0_H.raw = (WINDOW_0_H.raw & 0xFF00) | val;
    }
    else if(addr == 0x04000041){
        WINDOW_0_H.raw = (WINDOW_0_H.raw & 0xFF) | (HalfWord(val) << 8);
    }
    else if(addr == 0x04000042){
        WINDOW_1_H.raw = (WINDOW_1_H.raw & 0xFF00) | val;
    }
    else if(addr == 0x04000043){
        WINDOW_1_H.raw = (WINDOW_1_H.raw & 0xFF) | (HalfWord(val) << 8);
    }
    else if(addr == 0x04000044){
        WINDOW_0_V.raw = (WINDOW_0_V.raw & 0xFF00) | val;
    }
    else if(addr == 0x04000045){
        WINDOW_0_V.raw = (WINDOW_0_V.raw & 0xFF) | (HalfWord(val) << 8);
    }
    else if(addr == 0x04000046){
        WINDOW_1_V.raw = (WINDOW_1_V.raw & 0xFF00) | val;
    }
    else if(addr == 0x04000047){
        WINDOW_1_V.raw = (WINDOW_1_V.raw & 0xFF) | (HalfWord(val) << 8);
    }

    else if(addr == 0x04000048){
        WININ.raw = (WININ.raw & 0xFF00) | val;
    }
    else if(addr == 0x04000049){
        WININ.raw = (WININ.raw & 0xFF) | (HalfWord(val) << 8);
    }
    else if(addr == 0x0400004A){
        WINOUT.raw = (WINOUT.raw & 0xFF00) | val;
    }
    else if(addr == 0x0400004B){
        WINOUT.raw = (WINOUT.raw & 0xFF) | (HalfWord(val) << 8);
    }

    else if(addr == 0x04000050){
        SPECIAL_EFFECTS.raw = (SPECIAL_EFFECTS.raw & 0xFF00) | val;
    }
    else if(addr == 0x04000051){
        SPECIAL_EFFECTS.raw = (SPECIAL_EFFECTS.raw & 0xFF) | (HalfWord(val) << 8);
    }
    else if(addr == 0x04000052){
        ALPHA_BLENDING.raw = (ALPHA_BLENDING.raw & 0xFF00) | val;
    }
    else if(addr == 0x04000053){
        ALPHA_BLENDING.raw = (ALPHA_BLENDING.raw & 0xFF) | (HalfWord(val) << 8);
    }
    else if(addr == 0x04000054){
        BRIGHTNESS_FADE.raw = (BRIGHTNESS_FADE.raw & 0xFFFFFF00) | val;
    }
    else if(addr == 0x04000055){
        BRIGHTNESS_FADE.raw = (BRIGHTNESS_FADE.raw & 0xFFFF00FF) | (Word(val) << 8);
    }
    else if(addr == 0x04000056){
        BRIGHTNESS_FADE.raw = (BRIGHTNESS_FADE.raw & 0xFF00FFFF) | (Word(val) << 16);
    }
    else if(addr == 0x04000057){
        BRIGHTNESS_FADE.raw = (BRIGHTNESS_FADE.raw & 0x00FFFFFF) | (Word(val) << 24);
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
        
        case 0x04000048:
            return WININ.raw;
        case 0x04000049:
            return WININ.raw >> 8;
        case 0x0400004A:
            return WINOUT.raw;
        case 0x0400004B:
            return WINOUT.raw >> 8;

        case 0x04000050:
            return SPECIAL_EFFECTS.raw;
        case 0x04000051:
            return SPECIAL_EFFECTS.raw >> 8;
        case 0x04000052:
            return ALPHA_BLENDING.raw;
        case 0x04000053:
            return ALPHA_BLENDING.raw >> 8;
        case 0x04000054:
            return BRIGHTNESS_FADE.raw;
        case 0x04000055:
            return BRIGHTNESS_FADE.raw >> 8;
        case 0x04000056:
            return BRIGHTNESS_FADE.raw >> 16;
        case 0x04000057:
            return BRIGHTNESS_FADE.raw >> 24;
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