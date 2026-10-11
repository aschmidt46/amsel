#include "bus7.h"
#include "bus.h"
#include <iostream>
#include "framework/stringlib.h"

using namespace nds;

void nds::Bus7::init()
{
    new (&cpu) gba::CPU(this, false);
    // extrem wichtig bei cartridge boot time:
    cpu.initialNDS();

    for(int i = 0; i < 4; i++){
        new (&timers[i]) Timer<Bus7>(i, sharedBus, this, NDSEVENT_TimerOverflow7);
        new (&dma[i]) DMAChannel<Bus7>(i, sharedBus, this, NDSEVENT_DmaTransfer7);
    }
}

void nds::Bus7::writeByte(Word addr, Byte val)
{
    writeByteFromWide(addr, val);
}

Byte nds::Bus7::readByte(Word addr)
{
    return readByteFromWide(addr);
}

void nds::Bus7::writeByteFromWide(Word addr, Byte val)
{
    if(addr >= 0x02000000 && addr < 0x03000000){
        // main mem
        sharedBus->writeByte(addr, val);
    }
    // Shared WRAM
    if(addr >= 0x03000000 && addr < 0x03800000){
        const auto [m, o] = sharedBus->wramAllocationNDS7[sharedBus->wramCNT];
        addr -= 0x03000000;
        if(m == 0){ // keine Allokation -> Mirrors von arm7WRAM
            arm7WRAM[addr % 0x10000] = val;
        }
        else{
            sharedBus->sharedWRAM[o + (addr % m)] = val;
        }
    }
    else if(addr >= 0x03800000 && addr < 0x04000000){
        // main mem
        addr -= 0x03800000;
        arm7WRAM[addr % 0x10000] = val;
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
    else if(addr >= 0x04000100 && addr < 0x04000110){
        addr -= 0x04000100;
        const Word num = addr / 4;
        const Word reg = addr % 4;
        timers[num].onWrite(reg, val);
    }
    else if(addr >= 0x04000132 && addr < 0x04000134){
        sharedBus->KEYCNT.OnWriteByte(addr, val);
    }
    else if(addr >= 0x4000180 && addr < 0x4000188){
        sharedBus->ipc->writeReg(addr, val, false);
    }

    else if(addr == 0x04000204){
        exMem06 = 0x7F & val;
    }
    else if(addr == 0x4000206){
        WIFIWAITCNT = val & 0b111111;
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
    else if(addr == 0x4000301){
        HALTCNT = val & 0b11000000;
    }
    else if(addr == 0x4000304){
        POWCNT2 = val & 0b11;
    }

    if(addr >= 0x06000000 && addr < 0x07000000){
        addr -= 0x06000000;
        sharedBus->lcd->writeMemoryNDS7(addr % 0x4'0000, val);
    }
}

Byte nds::Bus7::readByteFromWide(Word addr)
{
    if(addr < 0x01000000){
        return bios7[addr % bios7.size()];
    }
    if(addr >= 0x02000000 && addr < 0x03000000){
        // main mem
        return sharedBus->readByte(addr);
    }
    // Shared WRAM
    if(addr >= 0x03000000 && addr < 0x03800000){
        const auto [m, o] = sharedBus->wramAllocationNDS7[sharedBus->wramCNT];
        addr -= 0x03000000;
        if(m == 0){ // keine Allokation -> Mirrors von arm7WRAM
            return arm7WRAM[addr % 0x10000];
        }
        else{
            return sharedBus->sharedWRAM[o + (addr % m)];
        }
    }
    if(addr >= 0x03800000 && addr < 0x04000000){
        // main mem
        addr -= 0x03800000;
        return arm7WRAM[addr % 0x10000];
    }

    if(addr >= 0x04000004 && addr < 0x04000008){
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

    if(addr >= 0x04000100 && addr < 0x04000110){
        addr -= 0x04000100;
        const Word num = addr / 4;
        const Word reg = addr % 4;
        return timers[num].onRead(reg);
    }

    if(addr >= 0x4000180 && addr < 0x4000188){
        return sharedBus->ipc->readReg(addr, false);
    }

    if(addr >= 0x04000130 && addr < 0x04000132){
        return sharedBus->KEYINPUT.OnReadByte(addr);
    }
    if(addr >= 0x04000132 && addr < 0x04000134){
        return sharedBus->KEYCNT.OnReadByte(addr);
    }
    if(addr >= 0x04000136 && addr < 0x04000138){
        return sharedBus->EXKEYINPUT.OnReadByte(addr);
    }

    // EXMEMCNT
    if(addr == 0x04000204){
        return (exMem06 & 0x7F) | (sharedBus->EXMEMCNT & 0x80);
    }
    if(addr == 0x04000205){
        return sharedBus->EXMEMCNT >> 8;
    }
    if(addr == 0x4000206){
        return WIFIWAITCNT;
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

    if(addr == 0x4000240){
        return sharedBus->lcd->getVRAMSTAT();
    }

    if(addr == 0x4000241){
        return sharedBus->wramCNT;
    }
    if(addr == 0x4000300){
        return POSTFLG;
    }
    if(addr == 0x4000301){
        return HALTCNT;
    }
    if(addr == 0x4000304){
        return POWCNT2;
    }

    if(addr >= 0x06000000 && addr < 0x07000000){
        addr -= 0x06000000;
        return sharedBus->lcd->readMemoryNDS7(addr % 0x4'0000);
    }

    std::cout << "Bus7 unknown Read: " << getHex0x(addr, 8) << "\n";
    // throw;

    return 0;
}

void nds::Bus7::writeHalfWord(Word addr, HalfWord val)
{
    writeByteFromWide(addr, val);
    writeByteFromWide(addr + 1, val >> 8);
}

HalfWord nds::Bus7::readHalfWord(Word addr)
{
    const HalfWord A1 = readByteFromWide(addr);
    const HalfWord A2 = readByteFromWide(addr + 1);
    return A1 | (A2 << 8);
}

void nds::Bus7::writeWord(Word addr, Word val)
{
    if(addr == 0x4000188){
        sharedBus->ipc->writeFIFOSEND(false, val);
    }
    else{
        writeByteFromWide(addr, val);
        writeByteFromWide(addr + 1, val >> 8);
        writeByteFromWide(addr + 2, val >> 16);
        writeByteFromWide(addr + 3, val >> 24);
    }
}

Word nds::Bus7::readWord(Word addr)
{
    if(addr == 0x4100000){
        return sharedBus->ipc->readFIFORECV(false);
    }

    const Word A1 = readByteFromWide(addr);
    const Word A2 = readByteFromWide(addr + 1);
    const Word A3 = readByteFromWide(addr + 2);
    const Word A4 = readByteFromWide(addr + 3);
    return A1 | (A2 << 8) | (A3 << 16) | (A4 << 24);
}

void nds::Bus7::PPUEnteredHBlank()
{
}

void nds::Bus7::PPUEnteredVBlank()
{
    for(auto &d : dma){
        if(d.getStartTiming() == DMA_VBLANK && d.isEnabled()){
            d.commenceTransfer();
            return;
        }
    }
}

void nds::Bus7::clock()
{
    clocks++;
    cpu.clock();
}

void nds::Bus7::addCPUCycles(size_t num)
{
    cpu.addCycles(num);
}

void nds::Bus7::loadBios(const std::vector<Byte> &content)
{
    bios7 = content;
}

size_t nds::Bus7::getBiosSize()
{
    return bios7.size();
}

size_t nds::Bus7::getClocks()
{
    return clocks;
}

void nds::Bus7::timerOverflowed(int index)
{
    if(index < 3){
        if(timers[index + 1].usesPreviousTimer()){
            timers[index + 1].clockWithPrevious();
        }
    }
}

void nds::Bus7::setIF(int bit, bool val)
{
    IF = (IF & ~(1u << bit)) | (val << bit);
}

Word nds::Bus7::getIE()
{
    return IE;
}

Word nds::Bus7::getIF()
{
    return IF;
}

bool nds::Bus7::hasIME()
{
    return IME.raw & 1;
}

void nds::Bus7::setHalt()
{
    sharedBus->setHalt();
}
