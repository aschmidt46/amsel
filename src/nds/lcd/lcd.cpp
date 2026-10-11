#include "lcd.h"
#include "../bus.h"

using namespace nds;

void nds::LCD::init()
{
    engineA = PPU(this, 0, false);
    engineB = PPU(this, 1, true);

    // default: LCDC mapping
    switchVramBank<0>();
    switchVramBank<1>();
    switchVramBank<2>();
    switchVramBank<3>();
    switchVramBank<4>();
    switchVramBank<5>();
    switchVramBank<6>();
    switchVramBank<8>();
    switchVramBank<9>();
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
    if(addr >= 0x04000000 && addr < 0x0400006E){
        return engineA.readPPURegister(addr);
    }
    if(addr >= 0x04001000 && addr < 0x0400106E){
        return engineB.readPPURegister(addr - 0x1000);
    }
    return 0;
}

void nds::LCD::writeRegister(Word addr, Byte val)
{
    if(addr == 0x04000004){
        DISPSTAT.raw = (DISPSTAT.raw & 0xFF07) | (val & 0b11111000); // untere 3 bits read only
    }
    else if(addr == 0x04000005){
        DISPSTAT.raw = (DISPSTAT.raw & 0x00FF) | (HalfWord(val) << 8);
    }

    else if(addr >= 0x04000000 && addr < 0x0400006E){
        engineA.writePPURegister(addr, val);
    }
    else if(addr >= 0x04001000 && addr < 0x0400106E){
        engineB.writePPURegister(addr - 0x1000, val);
    }
}

Byte nds::LCD::readMemory(Word addr)
{
    if(addr >= 0x05000000 && addr < 0x05000400){
        return engineA.paletteRam[addr - 0x05000000];
    }
    if(addr >= 0x05000400 && addr < 0x05000800){
        return engineB.paletteRam[addr - 0x05000400];
    }
    if(addr >= 0x07000000 && addr < 0x05000400){
        return engineA.paletteRam[addr - 0x05000000];
    }
    if(addr >= 0x06000000 && addr < 0x07000000){
        addr -= 0x06000000;
        const Word offset = addr % 0x4000;
        const Word page = addr / 0x4000;
        if(pageTable[page].state.valid){
            return vram[pageTable[page].state.bankID * 0x4000 + offset];
        }
    }
    if(addr >= 0x07000000 && addr < 0x07000400){
        return engineA.oamAttribs[addr - 0x07000000];
    }
    if(addr >= 0x07000400 && addr < 0x07000800){
        return engineB.oamAttribs[addr - 0x07000400];
    }
    return 0;
}

Word nds::LCD::getVramOffset(Word addr)
{
    // Adresse startet bei 0
    const Word offset = addr % 0x4000;
    const Word page = addr / 0x4000;
    if(pageTable[page].state.valid){
        return pageTable[page].state.bankID * 0x4000 + offset;
    }
    return 0;
}

void nds::LCD::writeMemory(Word addr, Byte val)
{
    if(addr >= 0x05000000 && addr < 0x05000400){
        engineA.paletteRam[addr - 0x05000000] = val;
    }
    else if(addr >= 0x05000400 && addr < 0x05000800){
        engineB.paletteRam[addr - 0x05000400] = val;
    }
    else if(addr >= 0x07000000 && addr < 0x05000400){
        engineA.paletteRam[addr - 0x05000000] = val;
    }
    else if(addr >= 0x06000000 && addr < 0x07000000){
        addr -= 0x06000000;
        const Word offset = addr % 0x4000;
        const Word page = addr / 0x4000;
        if(pageTable[page].state.valid){
            vram[pageTable[page].state.bankID * 0x4000 + offset] = val;
        }
    }
    else if(addr >= 0x07000000 && addr < 0x07000400){
        engineA.oamAttribs[addr - 0x07000000] = val;
    }
    else if(addr >= 0x07000400 && addr < 0x07000800){
        engineB.oamAttribs[addr - 0x07000400] = val;
    }
}

Byte nds::LCD::readMemoryNDS7(Word addr)
{
    // Adresse startet bei 0
    const Word offset = addr % 0x4000;
    const Word page = addr / 0x4000;
    if(arm7PageTable[page].state.valid){
        return vram[arm7PageTable[page].state.bankID * 0x4000 + offset];
    }
    else{
        return 0;
    }
}

void nds::LCD::writeMemoryNDS7(Word addr, Byte val)
{
    // Adresse startet bei 0
    const Word offset = addr % 0x4000;
    const Word page = addr / 0x4000;
    if(arm7PageTable[page].state.valid){
        vram[arm7PageTable[page].state.bankID * 0x4000 + offset] = val;
    }
}

uint32_t* nds::LCD::accessFramebuffer()
{
    return framebuffer.data();
}
