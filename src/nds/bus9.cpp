#include "bus9.h"
#include "bus.h"

using namespace gba;

void nds::Bus9::init()
{
    KEYINPUT.raw = 0b1111111111;
    new (&cpu) CPU(this, &cp15);
}

void nds::Bus9::writeByte(Word addr, Byte val)
{
    writeByteFromWide(addr, val);
}

Byte nds::Bus9::readByte(Word addr)
{
    return readByteFromWide(addr);
}

void nds::Bus9::writeByteFromWide(Word addr, Byte val)
{
    const Word itcmEnd = cp15.itcmRegion();
    const auto [dtcmStart, dtcmEnd] = cp15.dtcmRegion();
    if(cp15.itcmIsEnabled() && addr < itcmEnd){
        itcm[addr % 0x8000] = val;
    }
    else if(cp15.dtcmIsEnabled() && addr >= dtcmStart && addr < dtcmEnd){
        addr -= dtcmStart;
        dtcm[addr % 0x4000] = val;
    }
    else if(addr >= 0x02000000 && addr < 0x03000000){
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
    else if(addr >= 0x04000000 && addr < 0x04000060){
        sharedBus->lcd->writeRegister(addr, val);
    }
    else if(addr >= 0x04000132 && addr < 0x04000134){
        KEYCNT.OnWriteByte(addr, val);
    }

    // IPC
    else if(addr >= 0x4000180 && addr < 0x4000188){
        sharedBus->ipc->writeReg(addr, val, true);
    }

    // Interrupt handling
    else if(addr >= 0x04000208 && addr < 0x0400020C){
        return IME.OnWriteByte(addr, val);
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
}

Byte nds::Bus9::readByteFromWide(Word addr)
{
    const Word itcmEnd = cp15.itcmRegion();
    const auto [dtcmStart, dtcmEnd] = cp15.dtcmRegion();
    if(cp15.itcmIsEnabled() && !cp15.itcmIsLoadMode() && addr < itcmEnd){
        return itcm[addr % 0x8000];
    }
    if(cp15.dtcmIsEnabled() && !cp15.dtcmIsLoadMode() && addr >= dtcmStart && addr < dtcmEnd){
        addr -= dtcmStart;
        return dtcm[addr % 0x4000];
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
    if(addr >= 0x04000000 && addr < 0x04000060){
        return sharedBus->lcd->readRegister(addr);
    }

    if(addr >= 0x04000130 && addr < 0x04000132){
        return KEYINPUT.OnReadByte(addr);
    }
    if(addr >= 0x04000132 && addr < 0x04000134){
        return KEYCNT.OnReadByte(addr);
    }
    // IPC
    if(addr >= 0x4000180 && addr < 0x4000188){
        return sharedBus->ipc->readReg(addr, true);
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


    if(addr == 0x4000247){
        return sharedBus->wramCNT;
    }

    if(addr >= 0x4000280 && addr < 0x40002C0){
        return maths.onRead(addr);
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

    return 0;
}

void nds::Bus9::writeHalfWord(Word addr, HalfWord val)
{
    if(addr >= 0x05000000 && addr < 0x08000000){
        sharedBus->lcd->writeMemory(addr, val);
        sharedBus->lcd->writeMemory(addr + 1, val >> 8);
    }
    else{
        writeByteFromWide(addr, val);
        writeByteFromWide(addr + 1, val >> 8);
    }
}

HalfWord nds::Bus9::readHalfWord(Word addr)
{
    const HalfWord A1 = readByteFromWide(addr);
    const HalfWord A2 = readByteFromWide(addr + 1);
    return A1 | (A2 << 8);
}

void nds::Bus9::writeWord(Word addr, Word val)
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
        writeByteFromWide(addr, val);
        writeByteFromWide(addr + 1, val >> 8);
        writeByteFromWide(addr + 2, val >> 16);
        writeByteFromWide(addr + 3, val >> 24);
    }
}

Word nds::Bus9::readWord(Word addr)
{
    if(addr == 0x4100000){
        return sharedBus->ipc->readFIFORECV(true);
    }

    const Word A1 = readByteFromWide(addr);
    const Word A2 = readByteFromWide(addr + 1);
    const Word A3 = readByteFromWide(addr + 2);
    const Word A4 = readByteFromWide(addr + 3);
    return A1 | (A2 << 8) | (A3 << 16) | (A4 << 24);
}

void nds::Bus9::clock()
{
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
