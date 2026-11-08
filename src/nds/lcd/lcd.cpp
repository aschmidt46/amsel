#include "lcd.h"
#include "../bus.h"
#include <iostream>
#include "framework/global.h"

using namespace nds;

void nds::LCD::worker()
{
    bool br = false;

    auto visitor = overload{
        [&](const MemoryWrite &e){
            writeSync(e.addr, e.val);
        },
        [&](const VRAMWrite &e){
            vramSync[e.write.addr] = e.write.val;
        },
        [&](const EngineCommand &e){
            (e.engine->*e.function)();
            if(e.engine == &engineB && e.function == &PPU::onVBlank){
                queueFrameFinished = true;
                queueFrameFinished.notify_one();
            }
        },
        [&](const PPUAllocationBGEvent &e){
            // return, wenn es eine Deallokation sein soll und die vorher allokierte Bank nicht mit der neuen übereinstimmt
            if(e.allocation.ptr == nullptr && e.engine->bgExtendedPalette[e.index].allocatedTo != e.allocation.allocatedTo) return;
            e.engine->bgExtendedPalette[e.index] = e.allocation;
        },
        [&](const PPUAllocationOBJEvent &e){
            if(e.allocation.ptr == nullptr && e.engine->objExtendedPalette.allocatedTo != e.allocation.allocatedTo) return;
            e.engine->objExtendedPalette = e.allocation;
        },
        [&](const PoisonPill &e){
            br = true;
            queueFrameFinished = true;
            queueFrameFinished.notify_one();
        }
    };

    while(!br){
        workerNotify.wait(false);
        workerNotify = false;

        RenderEvent e;
        while(q.try_dequeue(e)){
            std::visit(visitor, e);
        }

    }
}

void nds::LCD::init()
{
    engineA = PPU(this, 0, false, &engine3D);
    engineB = PPU(this, 1, true, nullptr);

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
    // renderA = std::thread(&PPU::renderScanline, &engineA);
    // renderB = std::thread(&PPU::renderScanline, &engineB);
    // engineA.renderScanline();
    // engineB.renderScanline();
    q.enqueue(EngineCommand{.engine = &engineA, .function = &PPU::renderScanline});
    q.enqueue(EngineCommand{.engine = &engineB, .function = &PPU::renderScanline});

    if(vCount % 16 == 0){
        workerNotify = true;
        workerNotify.notify_one();
    }

}

void nds::LCD::onHBlank()
{
    if(vCount < 192){
        q.enqueue(EngineCommand{.engine = &engineA, .function = &PPU::onHBlank});
        q.enqueue(EngineCommand{.engine = &engineB, .function = &PPU::onHBlank});
    }


    DISPSTAT_7.state.hBlankFlag = 1;
    DISPSTAT_9.state.hBlankFlag = 1;
    if(vCount < 192){

        if(DISPSTAT_7.state.hBlankIE){
            bus->bus7->setIF(1, true);
        }
        if(DISPSTAT_9.state.hBlankIE){
            bus->bus9->setIF(1, true);
        }
        bus->PPUEnteredHBlank();
    }
}

void nds::LCD::onVBlank()
{
    q.enqueue(EngineCommand{.engine = &engineA, .function = &PPU::onVBlank});
    q.enqueue(EngineCommand{.engine = &engineB, .function = &PPU::onVBlank});
    
    workerNotify = true;
    workerNotify.notify_one();

    queueFrameFinished.wait(false);
    queueFrameFinished = false;


    // Nachdem Worker fertig
    std::swap(framebuffer, backBuffer);

    // Vblank
    DISPSTAT_7.state.vBlankFlag = 1;
    DISPSTAT_9.state.vBlankFlag = 1;
    if(DISPSTAT_7.state.vBlankIE){
        bus->bus7->setIF(0, true);
    }
    if(DISPSTAT_9.state.vBlankIE){
        bus->bus9->setIF(0, true);
    }
    hasframe = true;

    if(swap3DBuffers){
        swap3DBuffers = false;
        engine3D.swapBuffers();
    }

    bus->PPUEnteredVBlank();
}

void nds::LCD::increment()
{

    if(vCount < 192){
        q.enqueue(EngineCommand{.engine = &engineA, .function = &PPU::increment});
        q.enqueue(EngineCommand{.engine = &engineB, .function = &PPU::increment});
    }
    else{
        engineA.increment();
        engineB.increment();
    }

    DISPSTAT_7.state.hBlankFlag = 0;
    DISPSTAT_9.state.hBlankFlag = 0;
    vCount++;
    if(vCount == 192) onVBlank();

    if(vCount >= 263){
        vCount = 0;

        DISPSTAT_7.state.vCounterFlag = 0;
        DISPSTAT_9.state.vCounterFlag = 0;
        if(vCount == DISPSTAT_7.getVCountSetting() && DISPSTAT_7.state.vCounterIE){
            bus->bus7->setIF(2, true);
            DISPSTAT_7.state.vCounterFlag = 1;
        }
        if(vCount == DISPSTAT_9.getVCountSetting() && DISPSTAT_9.state.vCounterIE){
            bus->bus9->setIF(2, true);
            DISPSTAT_9.state.vCounterFlag = 1;
        }
    }
    if(vCount < 263){
        if(vCount == DISPSTAT_7.getVCountSetting()){
            DISPSTAT_7.state.vCounterFlag = 1;
            if(DISPSTAT_7.state.vCounterIE)
                bus->bus7->setIF(2, true);
        }
        else{
            DISPSTAT_7.state.vCounterFlag = 0;
        }
        if(vCount == DISPSTAT_9.getVCountSetting()){
            DISPSTAT_9.state.vCounterFlag = 1;
            if(DISPSTAT_9.state.vCounterIE)
                bus->bus9->setIF(2, true);
        }
        else{
            DISPSTAT_9.state.vCounterFlag = 0;
        }
    }
    if(vCount >= 262){
        DISPSTAT_7.state.vBlankFlag = 0;
        DISPSTAT_9.state.vBlankFlag = 0;
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

Byte nds::LCD::readRegister(Word addr, bool isNDS9)
{
    if(addr == 0x04000004){
        if(isNDS9){
            return DISPSTAT_9.raw;
        }
        else{
            return DISPSTAT_7.raw;
        }
    }
    if(addr == 0x04000005){
        if(isNDS9){
            return DISPSTAT_9.raw >> 8;
        }
        else{
            return DISPSTAT_7.raw >> 8;
        }
    }
    if(addr == 0x04000006) return vCount;
    if(addr == 0x04000007) return vCount >> 8;

    // Müssen separat abgehandelt werden, wegen aRegs, bRegs
    if(addr == 0x04000067){ // DISPCAPCNT
        return 0;
    }
    if(addr >= 0x04000060 && addr < 0x04000062){ // DISP3DCNT
        return engine3D.readRegister(addr);
    }
    if(addr == 0x04000068){ // Main Memory Display
        return 0;
    }


    if(addr >= 0x04000000 && addr < 0x0400006C){
        return aRegisters[addr - 0x04000000];
    }
    if(addr == 0x0400006C){
        return masterBright.raw;
    }
    if(addr == 0x0400006D){
        return masterBright.raw >> 8;
    }
    if(addr >= 0x04001000 && addr < 0x0400106C){
        return bRegisters[addr - 0x04001000];
    }
    std::cout << "Unknwon LCD reg read " << addr << "\n";
    return 0;
}

void nds::LCD::writeRegister(Word addr, Byte val, bool isNDS9)
{
    if(addr == 0x04000004){
        if(isNDS9){
            DISPSTAT_9.raw = (DISPSTAT_9.raw & 0xFF07) | (val & 0b11111000); // untere 3 bits read only
        }
        else{
            DISPSTAT_7.raw = (DISPSTAT_7.raw & 0xFF07) | (val & 0b11111000);
        }
    }
    else if(addr == 0x04000005){
        if(isNDS9){
            DISPSTAT_9.raw = (DISPSTAT_9.raw & 0x00FF) | (HalfWord(val) << 8);
        }
        else{
            DISPSTAT_7.raw = (DISPSTAT_7.raw & 0x00FF) | (HalfWord(val) << 8);
        }
    }

    else if(addr >= 0x04000060 && addr < 0x04000062){ // DISP3DCNT (aktuell vollsynchron)
        engine3D.writeRegister(addr, val);
    }

    else if(addr >= 0x04000000 && addr < 0x0400006C){
        aRegisters[addr - 0x04000000] = val;
        if(vCount >= 192){
            engineA.writePPURegister(addr, val);
        }
        else{
            MemoryWrite t{.addr = addr, .val = val};
            q.enqueue(t);
        }
    }
    else if(addr == 0x0400006C){
        masterBright.state.factor = val & 0x1F;
    }
    else if(addr == 0x0400006D){
        masterBright.state.mode = val >> 6;
    }
    else if(addr >= 0x04001000 && addr < 0x0400106C){
        bRegisters[addr - 0x04001000] = val;
        if(vCount >= 192){
            engineB.writePPURegister(addr - 0x1000, val);
        }
        else{
            MemoryWrite t{.addr = addr, .val = val};
            q.enqueue(t);
        }
    }
}

void nds::LCD::writeSync(Word addr, Byte val)
{
    if(addr >= 0x04000000 && addr < 0x0400006C){
        engineA.writePPURegister(addr, val);
    }
    else if(addr >= 0x04001000 && addr < 0x0400106C){
        engineB.writePPURegister(addr - 0x1000, val);
    }
    else if(addr >= 0x05000000 && addr < 0x05000400){
        engineA.paletteRamSync[addr - 0x05000000] = val;
    }
    else if(addr >= 0x05000400 && addr < 0x05000800){
        engineB.paletteRamSync[addr - 0x05000400] = val;
    }
    else if(addr >= 0x07000000 && addr < 0x07000400){
        engineA.oamAttribsSync[addr - 0x07000000] = val;
    }
    else if(addr >= 0x07000400 && addr < 0x07000800){
        engineB.oamAttribsSync[addr - 0x07000400] = val;
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

Byte nds::LCD::readSyncedMemory(Word addr)
{
    if(addr >= 0x05000000 && addr < 0x05000400){
        return engineA.paletteRamSync[addr - 0x05000000];
    }
    if(addr >= 0x05000400 && addr < 0x05000800){
        return engineB.paletteRamSync[addr - 0x05000400];
    }
    if(addr >= 0x06000000 && addr < 0x07000000){
        addr -= 0x06000000;
        const Word offset = addr % 0x4000;
        const Word page = addr / 0x4000;
        if(pageTable[page].state.valid){
            // Hoffentlich ändert sich nicht der PageTable in der Mitte vom Frame?
            return vramSync[pageTable[page].state.bankID * 0x4000 + offset];
        }
    }
    if(addr >= 0x07000000 && addr < 0x07000400){
        return engineA.oamAttribsSync[addr - 0x07000000];
    }
    if(addr >= 0x07000400 && addr < 0x07000800){
        return engineB.oamAttribsSync[addr - 0x07000400];
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
    if(vCount >= 192){
        if(addr >= 0x05000000 && addr < 0x05000400){
            engineA.paletteRam[addr - 0x05000000] = val;
            engineA.paletteRamSync[addr - 0x05000000] = val;
        }
        else if(addr >= 0x05000400 && addr < 0x05000800){
            engineB.paletteRam[addr - 0x05000400] = val;
            engineB.paletteRamSync[addr - 0x05000400] = val;
        }
        else if(addr >= 0x06000000 && addr < 0x07000000){
            addr -= 0x06000000;
            const Word vramOffset = addr % 0x4000;
            const Word vramPage = addr / 0x4000;
            const Word vramIndex = pageTable[vramPage].state.bankID * 0x4000 + vramOffset;
            if(pageTable[vramPage].state.valid){
                vram[vramIndex] = val;
                vramSync[vramIndex] = val;
            }
        }
        else if(addr >= 0x07000000 && addr < 0x07000400){
            engineA.oamAttribs[addr - 0x07000000] = val;
            engineA.oamAttribsSync[addr - 0x07000000] = val;
        }
        else if(addr >= 0x07000400 && addr < 0x07000800){
            engineB.oamAttribs[addr - 0x07000400] = val;
            engineB.oamAttribsSync[addr - 0x07000400] = val;
        }
    }
    else{ // Nur Standard Speicher, Sync Speicher unberührt, dafür Transaktion merken
        if(addr < 0x06000000 || addr >= 0x07000000){
            MemoryWrite t{.addr = addr, .val = val};
            q.enqueue(t);
        }

        if(addr >= 0x05000000 && addr < 0x05000400){
            engineA.paletteRam[addr - 0x05000000] = val;
        }
        else if(addr >= 0x05000400 && addr < 0x05000800){
            engineB.paletteRam[addr - 0x05000400] = val;
        }
        else if(addr >= 0x06000000 && addr < 0x07000000){
            addr -= 0x06000000;
            const Word vramOffset = addr % 0x4000;
            const Word vramPage = addr / 0x4000;
            const Word vramIndex = pageTable[vramPage].state.bankID * 0x4000 + vramOffset;
            if(pageTable[vramPage].state.valid){
                vram[vramIndex] = val;
                q.enqueue(VRAMWrite{MemoryWrite{vramIndex, val}});
            }
        }
        else if(addr >= 0x07000000 && addr < 0x07000400){
            engineA.oamAttribs[addr - 0x07000000] = val;
        }
        else if(addr >= 0x07000400 && addr < 0x07000800){
            engineB.oamAttribs[addr - 0x07000400] = val;
        }
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
    const Word vramIndex = arm7PageTable[page].state.bankID * 0x4000 + offset;

    if(vCount >= 192){
        if(arm7PageTable[page].state.valid){
            vram[vramIndex] = val;
            vramSync[vramIndex] = val;
        }
    }
    else{
        if(arm7PageTable[page].state.valid){
            q.enqueue(VRAMWrite{MemoryWrite{vramIndex, val}});
            vram[vramIndex] = val;
        }
    }
}

uint32_t* nds::LCD::accessFramebuffer()
{
    return framebuffer;
}
