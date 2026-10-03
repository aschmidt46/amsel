#include "lcd.h"
#include "../bus.h"

using namespace nds;

void nds::LCD::renderScanline()
{
    engineA.renderScanline();
    engineB.renderScanline();
}

void nds::LCD::onHBlank()
{
    engineA.onHBlank();
    engineB.onHBlank();

    DISPSTAT.state.hBlankFlag = 1;
    if(vCount < 160){

        if(DISPSTAT.state.hBlankIE){
            bus->setIF(1, true);
        }
        bus->PPUEnteredHBlank();
    }
}

void nds::LCD::onVBlank()
{
    engineA.onVBlank();
    engineB.onVBlank();

    // Vblank
    DISPSTAT.state.vBlankFlag = 1;
    if(DISPSTAT.state.vBlankIE){
        bus->setIF(0, true);
    }
    hasframe = true;
    bus->PPUEnteredVBlank();
}

void nds::LCD::increment()
{
    engineA.increment();
    engineB.increment();

    DISPSTAT.state.hBlankFlag = 0;
    vCount++;

    if(vCount >= 228){
        vCount = 0;

        DISPSTAT.state.vCounterFlag = 0;
        if(vCount == DISPSTAT.state.vCountSetting && DISPSTAT.state.vCounterIE){
            bus->setIF(2, true);
            DISPSTAT.state.vCounterFlag = 1;
        }
    }
    else if(vCount == DISPSTAT.state.vCountSetting && DISPSTAT.state.vCounterIE){
        bus->setIF(2, true);
        DISPSTAT.state.vCounterFlag = 1;
    }
    if(vCount >= 227){
        DISPSTAT.state.vBlankFlag = 0;
        bus->PPULeftVBlank();
    }
    bus->PPULeftHBlank();
}

Word nds::LCD::getVCount()
{
    return Word();
}

bool nds::LCD::hasFrame()
{
    return false;
}
