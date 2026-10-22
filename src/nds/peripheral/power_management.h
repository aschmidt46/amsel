#pragma once
#include "../bus_types.h"

namespace nds{

    union PMCONTROL_T{
        struct{
            Byte soundEnable : 1;
            Byte soundMute_unused : 1;
            Byte lowerBacklight : 1;
            Byte upperBacklight : 1;
            Byte powerLEDBlinkEnable : 1;
            Byte powerLEDBlinkSpeed : 1;
            Byte systemPower : 1;
            Byte _unused : 1;
        } state;
        Byte raw;
    };

    class PowerManagement{
        PMCONTROL_T pmControl = {.state = {.soundEnable = 1, .lowerBacklight = 1, .upperBacklight = 1}};
        Byte batteryPowerLEDStatus = 0;
        Byte micAmplifierControl = 0;
        Byte micAmplifierGain = 0;
        Byte backlightLevels = 0;

        Byte index = 0;

        
        public:
        bool indexSelected = false;
        bool awaitRead = false;
        Byte read();
        void write(Byte val);
    };
}
