#include "lcd.h"
#include "../bus.h"

using namespace nds;

void nds::LCD::init()
{
    engineA = PPU(this, 0);
    engineB = PPU(this, 1);
}

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
    if(vCount < 192){

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

    if(vCount >= 263){
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
    if(vCount >= 262){
        DISPSTAT.state.vBlankFlag = 0;
        bus->PPULeftVBlank();
    }
    bus->PPULeftHBlank();
}

Word nds::LCD::getVCount()
{
    return vCount;
}

bool nds::LCD::hasFrame()
{
    bool tmp = hasframe;
    hasframe = false;
    return tmp;
}

Byte nds::LCD::readRegister(Word addr)
{
    if(addr == 0x04000004){
        return DISPSTAT.raw;
    }
    if(addr == 0x04000005){
        return DISPSTAT.raw >> 8;
    }
    return engineA.readPPURegister(addr);
}

void nds::LCD::writeRegister(Word addr, Byte val)
{
    if(addr == 0x04000004){
        DISPSTAT.raw = (DISPSTAT.raw & 0xFF07) | (val & 0b11111000); // untere 3 bits read only
    }
    else if(addr == 0x04000005){
        DISPSTAT.raw = (DISPSTAT.raw & 0x00FF) | (HalfWord(val) << 8);
    }

    else engineA.writePPURegister(addr, val);
}

Byte nds::LCD::readMemory(Word addr)
{
    return engineA.readPPUMemory(addr);
}

void nds::LCD::writeMemory(Word addr, Byte val)
{
    engineA.writePPUMemory(addr, val);
}

uint32_t* nds::LCD::accessFramebuffer()
{
    return framebuffer.data();
}
