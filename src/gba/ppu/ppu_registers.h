#pragma once
#include "../arm/bus_types.h"

#ifdef __GNUC__
#define PACK( __Declaration__ ) __Declaration__ __attribute__((__packed__))
#endif

#ifdef _MSC_VER
#define PACK( __Declaration__ ) __pragma( pack(push, 1) ) __Declaration__ __pragma( pack(pop))
#endif


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

        auto operator<=>(const BGCNT_T& b) const{
            return b.state.BGPriority <=> state.BGPriority;
        }
    };

    union WIN_H_T { // werte >240 bedeutet wert=240
        struct{
            HalfWord rightMostPlus1 : 8; //lsb
            HalfWord leftMost : 8;
        } state;
        HalfWord raw;
    };

    union WIN_V_T { // werte >160 bedeutet wert=160
        struct{
            HalfWord bottomMostPlus1 : 8; //lsb
            HalfWord topMost : 8;
        } state;
        HalfWord raw;
    };

    union WININ_T {
        struct{
            HalfWord win0BGEnableBits : 4; //lsb
            HalfWord win0ObjEnable : 1;
            HalfWord win0ColorSpecialEffect : 1;
            HalfWord _fill0 : 2;
            HalfWord win1BGEnableBits : 4;
            HalfWord win1ObjEnable : 1;
            HalfWord win1ColorSpecialEffect : 1;
            HalfWord _fill1 : 2;
        } state;
        HalfWord raw;
    };

    union WINOUT_T {
        struct{
            HalfWord winOutBGEnableBits : 4; //lsb
            HalfWord winOutObjEnable : 1;
            HalfWord winOutColorSpecialEffect : 1;
            HalfWord _fill0 : 2;
            HalfWord winObjBGEnableBits : 4;
            HalfWord winObjObjEnable : 1;
            HalfWord winObjColorSpecialEffect : 1;
            HalfWord _fill1 : 2;
        } state;
        HalfWord raw;
    };

    struct WINDOW_ACTIVES_T{
        bool enableBG0;
        bool enableBG1;
        bool enableBG2;
        bool enableBG3;
        bool enableObj;
        bool enableSpecialFX;
        bool bgActive(int i){
            switch(i){
                case 0:
                    return enableBG0;
                case 1:
                    return enableBG1;
                case 2:
                    return enableBG2;
                case 3:
                    return enableBG3;
                default:
                    return false;
            }
        }
    };

    union SPECIAL_EFFECTS_T {
        struct{
            HalfWord targetA_bg0 : 1;
            HalfWord targetA_bg1 : 1;
            HalfWord targetA_bg2 : 1;
            HalfWord targetA_bg3 : 1;
            HalfWord targetA_obj : 1;
            HalfWord targetA_bd : 1;
            HalfWord specialEffect : 2;
            HalfWord targetB_bg0 : 1;
            HalfWord targetB_bg1 : 1;
            HalfWord targetB_bg2 : 1;
            HalfWord targetB_bg3 : 1;
            HalfWord targetB_obj : 1;
            HalfWord targetB_bd : 1;
            HalfWord _fill1 : 2;
        } state;
        HalfWord raw;
    };

    union ALPHA_BLEND_COEF_T {
        struct{
            HalfWord coefA : 5;
            HalfWord _fill0 : 3;
            HalfWord coefB : 5;
            HalfWord _fill1 : 3;
        } state;
        HalfWord raw;
    };

    union BRIGHTNESS_FADE_T {
        struct{
            Word coef : 5;
            Word _fill : 27;
        } state;
        Word raw;
    };

    struct PIXEL_T{
        Byte pixel = 0; // 0 == BD
        Byte red = 0;
        Byte green = 0;
        Byte blue = 0;
        Byte priority = 0;
        Byte layerIndex = 99; // 0=sprite, 1-4 = bg0-3, 5 = bd

        auto operator<=>(const PIXEL_T& b) const{
            return b.priority <=> priority;
        }
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

    
    PACK(struct OAMEntry {
        struct{
            HalfWord TileID : 10; //lsb
            HalfWord flipHorizontal : 1;
            HalfWord flipVertical : 1;
            HalfWord paletteBank : 4;
        } state;
        HalfWord raw;
    });

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

    PACK(struct OAMAttribs {
        Attribute0_T attr0;
        Attribute1_T attr1;
        Attribute2_T attr2;
        HalfWord fill;

        auto operator<=>(const OAMAttribs& b) const{
            return b.attr2.state.priority <=> attr2.state.priority;
        }
    });

    PACK(struct AffineAttribs {
        HalfWord fill0[3];
        int16_t pa;
        HalfWord fill1[3];
        int16_t pb;
        HalfWord fill2[3];
        int16_t pc;
        HalfWord fill3[3];
        int16_t pd;
    });



    static_assert(sizeof(OAMAttribs) == 8, "Wrong Size of OAM Attributes");
    static_assert(sizeof(AffineAttribs) == 32, "Wrong Size of Affine Attributes");



}
