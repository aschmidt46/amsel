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
}
