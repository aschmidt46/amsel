#pragma once
#include "arm/bus_types.h"

namespace gba{
    union LCDCONTROL_T {
        struct{
            HalfWord bgMode : 3; //lsb
            HalfWord cgbMode : 1;
            HalfWord frameSelect : 1;
            HalfWord hblankIntervalFree : 1;
            HalfWord ObjCharVRAMMapping : 1;
            HalfWord ForcedBlank : 1;
            HalfWord displayBG0 : 1;
            HalfWord displayBG1 : 1;
            HalfWord displayBG2 : 1;
            HalfWord displayBG3 : 1;
            HalfWord displayOBJ : 1;
            HalfWord displayWin0 : 1;
            HalfWord displayWin1 : 1;
            HalfWord displayObjWindow : 1;
        } state;
        HalfWord raw;
    };

    union LCDSTATUS_T {
        struct{
            HalfWord vBlankFlag : 1; //lsb
            HalfWord hBlankFlag : 1;
            HalfWord vCounterFlag : 1;
            HalfWord vBlankIE : 1;
            HalfWord hBlankIE : 1;
            HalfWord vCounterIE : 1;
            HalfWord Unused1 : 1;
            HalfWord Unused2 : 1;
            HalfWord vCountSetting : 8;
        } state;
        HalfWord raw;
    };

    union BGCNT_T {
        struct{
            HalfWord BGPriority : 2; //lsb
            HalfWord CHRBaseBlock : 2;
            HalfWord unused : 2; // muss 0 sein
            HalfWord mosaic : 1;
            HalfWord colorsPalettes : 1;
            HalfWord screenBaseBlock : 5;
            // HalfWord unusedBG0BG1 : 1;
            HalfWord DisplayAreaOverflowBG2BG3 : 1;
            HalfWord screenSize : 2;
        } state;
        HalfWord raw;
    };

    union ScreenEntry {
        struct{
            HalfWord TileID : 10; //lsb
            HalfWord flipHorizontal : 1;
            HalfWord flipVertical : 1;
            HalfWord paletteBank : 4;
        } state;
        HalfWord raw;
    };

    
    struct __attribute__((__packed__)) OAMEntry {
        struct{
            HalfWord TileID : 10; //lsb
            HalfWord flipHorizontal : 1;
            HalfWord flipVertical : 1;
            HalfWord paletteBank : 4;
        } state;
        HalfWord raw;
    };

    union Attribute0_T {
        struct{
            HalfWord yCoord : 8;
            HalfWord objectMode : 2;
            HalfWord gfxMode : 2;
            HalfWord mosaic : 1;
            HalfWord colorMode : 1;
            HalfWord spriteShape : 2;
        } state;
        HalfWord raw;
    };

    union Attribute1_T {
        struct{
            HalfWord xCoord : 9;
            HalfWord affineIndex : 5;
            HalfWord spriteSize : 2;
            // Mehrdeutig, nur gültig falls der Sprite nicht affin ist
            inline HalfWord getHorizontalFlip(){
                return (affineIndex >> 3) & 1;
            }
            inline HalfWord getVerticalFlip(){
                return (affineIndex >> 4) & 1;
            }
        } state;
        HalfWord raw;
    };

    union Attribute2_T {
        struct{
            HalfWord baseTileIndex : 10;
            HalfWord priority : 2;
            HalfWord paletteBank : 4;
        } state;
        HalfWord raw;
    };

    struct __attribute__((__packed__)) OAMAttribs {
        Attribute0_T attr0;
        Attribute1_T attr1;
        Attribute2_T attr2;
        HalfWord fill;
    };

    struct __attribute__((__packed__)) AffineAttribs {
        HalfWord fill0[3];
        int16_t pa;
        HalfWord fill1[3];
        int16_t pb;
        HalfWord fill2[3];
        int16_t pc;
        HalfWord fill3[3];
        int16_t pd;
    };



    static_assert(sizeof(OAMAttribs) == 8, "Wrong Size of OAM Attributes");
    static_assert(sizeof(AffineAttribs) == 32, "Wrong Size of Affine Attributes");



}
