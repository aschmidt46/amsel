#include "bus9.h"
#include "bus.h"
#include <iostream>
#include "framework/stringlib.h"

using namespace gba;

void nds::Bus9::init()
{
    new (&cpu) CPU(this, &cp15);

    for(int i = 0; i < 4; i++){
        new (&timers[i]) Timer<Bus9>(i, sharedBus, this, NDSEVENT_TimerOverflow9);
        new (&dma[i]) DMAChannel<Bus9>(i, sharedBus, this, NDSEVENT_DmaTransfer9);
    }
}

void nds::Bus9::writeByte(Word addr, Byte val)
{
    writeByteFromWide(addr, val);
}

Byte nds::Bus9::readByte(Word addr)
{
    return readByteFromWide(addr);
}

void nds::Bus9::writeByteFromWide(Word addr, Byte val, bool isDMA)
{
    if(!isDMA){
        const Word itcmEnd = cp15.itcmRegion();
        const auto [dtcmStart, dtcmEnd] = cp15.dtcmRegion();
        if(cp15.itcmIsEnabled() && addr < itcmEnd){
            itcm[addr % 0x8000] = val;
            return;
        }
        else if(cp15.dtcmIsEnabled() && addr >= dtcmStart && addr < dtcmEnd){
            addr -= dtcmStart;
            dtcm[addr % 0x4000] = val;
            return;
        }
    }

    if(addr >= 0x02000000 && addr < 0x03000000){
        // main mem
        sharedBus->writeByte(addr, val);
    }
    // Shared WRAM
    else if(addr >= 0x03000000 && addr < 0x04000000){
        const auto [m, o] = sharedBus->wramAllocationNDS9[sharedBus->wramCNT];
        addr -= 0x03000000;
        if(m > 0){
            sharedBus->sharedWRAM[o + (addr % m)] = val;
        }
    }

     // LCD 2D
    else if(addr >= 0x04000000 && addr < 0x0400006E){
        sharedBus->lcd->writeRegister(addr, val);
    }
    // DMA
    else if(addr >= 0x040000B0 && addr < 0x040000BC){
        dma[0].onWrite(addr - 0x040000B0, val);
    }
    else if(addr >= 0x040000BC && addr < 0x040000C8){
        dma[1].onWrite(addr - 0x040000BC, val);
    }
    else if(addr >= 0x040000C8 && addr < 0x040000D4){
        dma[2].onWrite(addr - 0x040000C8, val);
    }
    else if(addr >= 0x040000D4 && addr < 0x040000E0){
        dma[3].onWrite(addr - 0x040000D4, val);
    }
    else if(addr >= 0x040000E0 && addr < 0x040000E4){
        dmaFill[0].OnWriteByte(addr, val);
    }
    else if(addr >= 0x040000E4 && addr < 0x040000E8){
        dmaFill[1].OnWriteByte(addr, val);
    }
    else if(addr >= 0x040000E8 && addr < 0x040000EC){
        dmaFill[2].OnWriteByte(addr, val);
    }
    else if(addr >= 0x040000EC && addr < 0x040000F0){
        dmaFill[3].OnWriteByte(addr, val);
    }
    else if(addr >= 0x04000100 && addr < 0x04000110){
        addr -= 0x04000100;
        const Word num = addr / 4;
        const Word reg = addr % 4;
        timers[num].onWrite(reg, val);
    }
    else if(addr >= 0x04000132 && addr < 0x04000134){
        sharedBus->KEYCNT.OnWriteByte(addr, val);
    }

    // IPC
    else if(addr >= 0x4000180 && addr < 0x4000188){
        sharedBus->ipc->writeReg(addr, val, true);
    }

    else if(addr == 0x04000204){
        exMem06 = 0x7F & val;
        Byte msb = 0x80 & val;
        sharedBus->EXMEMCNT = (sharedBus->EXMEMCNT & 0xFF00) | msb;
    }
    else if(addr == 0x04000205){
        sharedBus->EXMEMCNT = (sharedBus->EXMEMCNT & 0x00FF) | (HalfWord(val) << 8);
    }

    // Interrupt handling
    else if(addr >= 0x04000208 && addr < 0x0400020C){
        IME.OnWriteByte(addr, val);
    }
    else if(addr == 0x04000210){
        IE = (IE & 0xFFFFFF00) | val;
    }
    else if(addr == 0x04000211){//!!
        IE = (IE & 0xFFFF00FF) | (Word(val) << 8);
    }
    else if(addr == 0x04000212){//!!
        IE = (IE & 0xFF00FFFF) | (Word(val) << 16);
    }
    else if(addr == 0x04000213){//!!
        IE = (IE & 0x00FFFFFF) | (Word(val) << 24);
    }
    else if(addr == 0x04000214){
        IF = IF & ~Word(val);
    }
    else if(addr == 0x04000215){
        IF = IF & ~(Word(val) << 8);
    }
    else if(addr == 0x04000216){
        IF = IF & ~(Word(val) << 16);
    }
    else if(addr == 0x04000217){
        IF = IF & ~(Word(val) << 24);
    }

    else if(addr >= 0x4000240 && addr < 0x4000247){
        sharedBus->lcd->writeVramBankControl(addr, val);
    }
    else if(addr == 0x4000247){
        sharedBus->wramCNT = val & 0b11;
    }
    else if(addr >= 0x4000248 && addr < 0x400024A){
        sharedBus->lcd->writeVramBankControl(addr, val);
    }

    else if(addr >= 0x4000280 && addr < 0x40002C0){
        maths.onWrite(addr, val);
    }

    else if(addr == 0x4000304){ // Muss noch implementiert werden
        POWCNT1 = (POWCNT1 & 0xFF00) | (val & 0b1111);
        onWritePOWCNT1();
    }
    else if(addr == 0x4000305){
        POWCNT1 = (POWCNT1 & 0x00FF) | (HalfWord(val & 0b1000010) << 8);
        onWritePOWCNT1();
    }
    // Engine B
    else if(addr >= 0x04001000 && addr < 0x0400106E){
        sharedBus->lcd->writeRegister(addr, val);
    }
}

Byte nds::Bus9::readByteFromWide(Word addr, bool isDMA)
{
    if(!isDMA){
        const Word itcmEnd = cp15.itcmRegion();
        const auto [dtcmStart, dtcmEnd] = cp15.dtcmRegion();
        if(cp15.itcmIsEnabled() && !cp15.itcmIsLoadMode() && addr < itcmEnd){
            return itcm[addr % 0x8000];
        }
        if(cp15.dtcmIsEnabled() && !cp15.dtcmIsLoadMode() && addr >= dtcmStart && addr < dtcmEnd){
            addr -= dtcmStart;
            return dtcm[addr % 0x4000];
        }
    }

    // MAIN MEM
    if(addr >= 0x02000000 && addr < 0x03000000){
        return sharedBus->readByte(addr);
    }

    // Shared WRAM
    if(addr >= 0x03000000 && addr < 0x04000000){
        const auto [m, o] = sharedBus->wramAllocationNDS9[sharedBus->wramCNT];
        addr -= 0x03000000;
        if(m == 0){ // keine Allokation -> Undefinierte Daten
            return 0;
        }
        else{
            return sharedBus->sharedWRAM[o + (addr % m)];
        }
    }

    // LCD 2D
    if(addr >= 0x04000000 && addr < 0x0400006E){
        return sharedBus->lcd->readRegister(addr);
    }

    // DMA
    if(addr >= 0x040000B0 && addr < 0x040000BC){
        return dma[0].onRead(addr - 0x040000B0);
    }
    if(addr >= 0x040000BC && addr < 0x040000C8){
        return dma[1].onRead(addr - 0x040000BC);
    }
    if(addr >= 0x040000C8 && addr < 0x040000D4){
        return dma[2].onRead(addr - 0x040000C8);
    }
    if(addr >= 0x040000D4 && addr < 0x040000E0){
        return dma[3].onRead(addr - 0x040000D4);
    }
    if(addr >= 0x040000E0 && addr < 0x040000E4){
        return dmaFill[0].OnReadByte(addr);
    }
    if(addr >= 0x040000E4 && addr < 0x040000E8){
        return dmaFill[1].OnReadByte(addr);
    }
    if(addr >= 0x040000E8 && addr < 0x040000EC){
        return dmaFill[2].OnReadByte(addr);
    }
    if(addr >= 0x040000EC && addr < 0x040000F0){
        return dmaFill[3].OnReadByte(addr);
    }

    if(addr >= 0x04000100 && addr < 0x04000110){
        addr -= 0x04000100;
        const Word num = addr / 4;
        const Word reg = addr % 4;
        return timers[num].onRead(reg);
    }

    if(addr >= 0x04000130 && addr < 0x04000132){
        return sharedBus->KEYINPUT.OnReadByte(addr);
    }
    if(addr >= 0x04000132 && addr < 0x04000134){
        return sharedBus->KEYCNT.OnReadByte(addr);
    }
    // IPC
    if(addr >= 0x4000180 && addr < 0x4000188){
        return sharedBus->ipc->readReg(addr, true);
    }

    // EXMEMCNT
    if(addr == 0x04000204){
        return (exMem06 & 0x7F) | (sharedBus->EXMEMCNT & 0x80);
    }
    if(addr == 0x04000205){
        return sharedBus->EXMEMCNT >> 8;
    }

    //Interrupt Handling
    if(addr >= 0x04000208 && addr < 0x0400020C){
        return IME.OnReadByte(addr);
    }
    if(addr == 0x04000210){
        return IE;
    }
    if(addr == 0x04000211){//!!
        return IE >> 8;
    }
    if(addr == 0x04000212){//!!
        return IE >> 16;
    }
    if(addr == 0x04000213){//!!
        return IE >> 24;
    }
    if(addr == 0x04000214){
        return IF;
    }
    if(addr == 0x04000215){
        return IF >> 8;
    }
    if(addr == 0x04000216){
        return IF >> 16;
    }
    if(addr == 0x04000217){
        return IF >> 24;
    }


    if(addr >= 0x4000240 && addr < 0x4000247){
        return sharedBus->lcd->readVramBankControl(addr);
    }
    if(addr == 0x4000247){
        return sharedBus->wramCNT;
    }
    if(addr >= 0x4000248 && addr < 0x400024A){
        return sharedBus->lcd->readVramBankControl(addr);
    }

    if(addr >= 0x4000280 && addr < 0x40002C0){
        return maths.onRead(addr);
    }
    if(addr == 0x4000300){ // --------------------------------------- Achtung, bit 1 ist schreibbar aus BIOS, siehe Gbatek
        return POSTFLG;
    }
    if(addr == 0x4000304){ // Muss noch implementiert werden
        return POWCNT1;
    }
    if(addr == 0x4000305){
        return POWCNT1 >> 8;
    }

    // Engine B
    if(addr >= 0x04001000 && addr < 0x0400106E){
        return sharedBus->lcd->readRegister(addr);
    }

    // ACHTUNG
    if(addr >= 0x05000000 && addr < 0x08000000){
        return sharedBus->lcd->readMemory(addr);
    }

    // BIOS
    if(addr >= 0xFFFF0000){
        addr -= 0xFFFF0000;
        return bios9[addr % bios9.size()];
    }

    std::cout << "Bus9 unknown Read: " << getHex0x(addr, 8) << "\n";

    return 0;
}

void nds::Bus9::writeHalfWord(Word addr, HalfWord val, bool isDMA)
{
    if(addr >= 0x05000000 && addr < 0x08000000){
        sharedBus->lcd->writeMemory(addr, val);
        sharedBus->lcd->writeMemory(addr + 1, val >> 8);
    }
    else{
        writeByteFromWide(addr, val, isDMA);
        writeByteFromWide(addr + 1, val >> 8, isDMA);
    }
}

HalfWord nds::Bus9::readHalfWord(Word addr, bool isDMA)
{
    const HalfWord A1 = readByteFromWide(addr, isDMA);
    const HalfWord A2 = readByteFromWide(addr + 1, isDMA);
    return A1 | (A2 << 8);
}

void nds::Bus9::writeWord(Word addr, Word val, bool isDMA)
{
    if(addr == 0x4000188){
        sharedBus->ipc->writeFIFOSEND(true, val);
    }
    else if(addr >= 0x05000000 && addr < 0x08000000){
        sharedBus->lcd->writeMemory(addr, val);
        sharedBus->lcd->writeMemory(addr + 1, val >> 8);
        sharedBus->lcd->writeMemory(addr + 2, val >> 16);
        sharedBus->lcd->writeMemory(addr + 3, val >> 24);
    }
    else{
        writeByteFromWide(addr, val, isDMA);
        writeByteFromWide(addr + 1, val >> 8, isDMA);
        writeByteFromWide(addr + 2, val >> 16, isDMA);
        writeByteFromWide(addr + 3, val >> 24, isDMA);
    }
}

Word nds::Bus9::readWord(Word addr, bool isDMA)
{
    if(addr == 0x4100000){
        return sharedBus->ipc->readFIFORECV(true);
    }

    const Word A1 = readByteFromWide(addr, isDMA);
    const Word A2 = readByteFromWide(addr + 1, isDMA);
    const Word A3 = readByteFromWide(addr + 2, isDMA);
    const Word A4 = readByteFromWide(addr + 3, isDMA);
    return A1 | (A2 << 8) | (A3 << 16) | (A4 << 24);
}

void nds::Bus9::PPUEnteredHBlank()
{
    int block = 5;
    for(int i = 0; i < 4; i++){
        if(dma[i].getStartTiming() == DMA_HBLANK_OR_CARTRIDGE_SLOT && dma[i].isEnabled()){ // HBlank dma
            block = i;
            break;
        }
    }
    if(block < 5){
        dma[block].commenceTransfer();
    }
}

void nds::Bus9::PPUEnteredVBlank()
{
    for(auto &d : dma){
        if(d.getStartTiming() == DMA_VBLANK && d.isEnabled()){
            d.commenceTransfer();
            return;
        }
    }
}

void nds::Bus9::clock()
{
    clocks++;
    // 66 MHz
    cpu.clock();
    cpu.clock();
    maths.clock();

}

void nds::Bus9::addCPUCycles(size_t num)
{
    cpu.addCycles(num);
}

void nds::Bus9::loadBios(const std::vector<Byte> &content)
{
    bios9 = content;
}

size_t nds::Bus9::getBiosSize()
{
    return bios9.size();
}

size_t nds::Bus9::getClocks()
{
    return clocks;
}

void nds::Bus9::timerOverflowed(int index)
{
    if(index < 3){
        if(timers[index + 1].usesPreviousTimer()){
            timers[index + 1].clockWithPrevious();
        }
    }
}

void nds::Bus9::setIF(int bit, bool val)
{
    IF = (IF & ~(1u << bit)) | (val << bit);
}

Word nds::Bus9::getIE()
{
    return IE;
}

Word nds::Bus9::getIF()
{
    return IF;
}

bool nds::Bus9::hasIME()
{
    return IME.raw & 1;
}

void nds::Bus9::setHalt()
{
    sharedBus->setHalt();
}

void nds::Bus9::onWritePOWCNT1()
{
    bool dispSwap = (POWCNT1 & (1u << 15)) > 0;
    if(dispSwap){
        sharedBus->lcd->engineA.framebufferOffset = 0;
        sharedBus->lcd->engineB.framebufferOffset = 1;
    }
    else{
        sharedBus->lcd->engineA.framebufferOffset = 1;
        sharedBus->lcd->engineB.framebufferOffset = 0;
    }
}
