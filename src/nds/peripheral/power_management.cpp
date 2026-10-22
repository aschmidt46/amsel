#include "power_management.h"

using namespace nds;

Byte nds::PowerManagement::read()
{
    if(awaitRead){
        awaitRead = false;
        return 0;
    }

    if(indexSelected && (index >> 7)){
        // indexSelected = false;
        Byte i = (index & 0x7F) % 8;
        if(i >= 5) i = 4;

        switch(i){
            case 0: return pmControl.raw;
            case 1: return batteryPowerLEDStatus;
            case 2: return micAmplifierControl;
            case 3: return micAmplifierGain;
            case 4: return backlightLevels;
            default: return 0;
        }
    }
    return 0;
}

void nds::PowerManagement::write(Byte val)
{
    if(!indexSelected){
        index = val;
        indexSelected = true;
        if(index >> 7) awaitRead = true;
        return;
    }
    else if(!(index >> 7)){ // write, index selected
        // indexSelected = false;
        Byte i = (index & 0x7F) % 8;
        if(i >= 5) i = 4;

        switch(i){
            case 0:  pmControl.raw = val; return;
            case 1:  batteryPowerLEDStatus = val; return;
            case 2:  micAmplifierControl = val; return;
            case 3:  micAmplifierGain = val; return;
            case 4:  backlightLevels = val; return;
            default: return;
        }
    }
}
