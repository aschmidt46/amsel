#include "bus7.h"
#include "bus.h"
#include <iostream>
#include "framework/stringlib.h"

using namespace nds;

void nds::Bus7::setInterruptPending()
{
    interruptPending = IF & IE;
}

void nds::Bus7::init()
{
    new (&cpu) gba::CPU(this, false);

    new (&spi) SPI(this);

    rtc = std::make_unique<RTC>(this);

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
    else if(addr >= 0x03000000 && addr < 0x03800000){
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

    else if(addr >= 0x04000004 && addr < 0x04000008){
        sharedBus->lcd->writeRegister(addr, val, false);
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
    else if(addr == 0x04000134){
        RCNT = (RCNT & 0xFF00) | (val);
    }
    else if(addr == 0x04000135){
        RCNT = (RCNT & 0x00FF) | (HalfWord(val) << 8);
    }

    else if(addr == 0x04000138){
        rtc->onWrite(addr ,val);
    }

    else if(addr >= 0x4000180 && addr < 0x4000188){
        sharedBus->ipc->writeReg(addr, val, false);
    }

    // Gamecart
    else if(addr >= 0x040001A0 && addr < 0x040001B0 && sharedBus->canAccessGameCart(false)){
        sharedBus->gameCart.onWrite(addr, val);
    }

    // SPI
    else if(addr >= 0x040001C0 && addr < 0x040001C3){
        spi.onWrite(addr, val);
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
        setInterruptPending();
    }
    else if(addr == 0x04000211){//!!
        IE = (IE & 0xFFFF00FF) | (Word(val) << 8);
        setInterruptPending();
    }
    else if(addr == 0x04000212){//!!
        IE = (IE & 0xFF00FFFF) | (Word(val) << 16);
        setInterruptPending();
    }
    else if(addr == 0x04000213){//!!
        IE = (IE & 0x00FFFFFF) | (Word(val) << 24);
        setInterruptPending();
    }
    else if(addr == 0x04000214){
        IF = IF & ~Word(val);
        setInterruptPending();
    }
    else if(addr == 0x04000215){
        IF = IF & ~(Word(val) << 8);
        setInterruptPending();
    }
    else if(addr == 0x04000216){
        IF = IF & ~(Word(val) << 16);
        setInterruptPending();
    }
    else if(addr == 0x04000217){
        IF = IF & ~(Word(val) << 24);
        setInterruptPending();
    }
    else if(addr == 0x4000300){
        POSTFLG = val & 1;
    }
    else if(addr == 0x4000301){
        HALTCNT = val & 0b11000000;
        if((HALTCNT >> 6) == 2) cpu.enterHaltMode();
    }
    else if(addr == 0x4000304){
        POWCNT2 = val & 0b11;
    }

    else if(addr >= 0x04000400 && addr < 0x04000500){
        audioStub[addr - 0x04000400] = val;
    }

    else if(addr == 0x04000500){
        SOUNDCNTSTUB = (SOUNDCNTSTUB & 0xFF00) | (val);
    }
    else if(addr == 0x04000501){
        SOUNDCNTSTUB = (SOUNDCNTSTUB & 0x00FF) | (HalfWord(val) << 8);
    }
    else if(addr == 0x4000504){
        SOUNDBIASSTUB = (SOUNDBIASSTUB & 0xFF00) | val;
    }
    else if(addr == 0x4000505){
        SOUNDBIASSTUB = (SOUNDBIASSTUB & 0x00FF) | (HalfWord(val & 0b11) << 8);
    }

    // Gamecart
    else if(addr >= 0x04100010 && addr < 0x04100014 && sharedBus->canAccessGameCart(false)){
        sharedBus->gameCart.onWrite(addr, val);
    }

    else if(addr >= 0x4800000 && addr < 0x4810000){
        addr -= 0x4800000;
        wifi[addr % 0x8000] = val;
    }

    else if(addr >= 0x06000000 && addr < 0x07000000){
        addr -= 0x06000000;
        sharedBus->lcd->writeMemoryNDS7(addr % 0x4'0000, val);
    }
    // else if(addr == 0x040001A8){
    //     std::cout << "Gamecard Command sent from NDS7\n";
    // }
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
        return sharedBus->lcd->readRegister(addr, false);
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
    if(addr == 0x04000134){
        return RCNT;
    }
    if(addr == 0x04000135){
        return RCNT >> 8;
    }
    if(addr == 0x04000136){
        return sharedBus->EXKEYINPUT;
    }
    if(addr == 0x04000138){
        return rtc->onRead(addr);
    }

    // Gamecart
    if(addr >= 0x040001A0 && addr < 0x040001B0 && sharedBus->canAccessGameCart(false)){
        return sharedBus->gameCart.onRead(addr);
    }

    // SPI
    if(addr >= 0x040001C0 && addr < 0x040001C4){
        return spi.onRead(addr);
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
    if(addr == 0x04000208){
        return IME.OnReadByte(addr) & 1u;
    }
    if(addr >= 0x04000209 && addr < 0x04000210) return 0;
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
    if(addr >= 0x4000308 && addr < 0x400030C){
        return BIOSPROT.OnReadByte(addr);
    }

    if(addr >= 0x04000400 && addr < 0x04000500){
        return 0;
        return audioStub[addr - 0x04000400];
    }

    if(addr == 0x04000500){
        return SOUNDCNTSTUB;
    }
    if(addr == 0x04000501){
        return SOUNDCNTSTUB >> 8;
    }
    if(addr == 0x4000504){
        return SOUNDBIASSTUB;
    }
    if(addr == 0x4000505){
        return SOUNDBIASSTUB >> 8;
    }

    // Gamecart
    if(addr >= 0x04100010 && addr < 0x04100014 && sharedBus->canAccessGameCart(false)){
        return sharedBus->gameCart.onRead(addr);
    }

    if(addr == 0x480803C){
        return 0b00000010;
    }
    if(addr == 0x480803D){
        return 0b00000010;
    }

    if(addr >= 0x4800000 && addr < 0x4810000){
        addr -= 0x4800000;
        return wifi[addr % 0x8000];
    }

    if(addr >= 0x06000000 && addr < 0x07000000){
        addr -= 0x06000000;
        return sharedBus->lcd->readMemoryNDS7(addr % 0x4'0000);
    }

    // std::cout << "Bus7 unknown Read: " << getHex0x(addr, 8) << "\n";

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

void nds::Bus7::onDataWordArrived()
{
    for(auto &d : dma){
        if(d.getStartTiming() == DMA_HBLANK_OR_CARTRIDGE_SLOT && d.isEnabled()){
            d.commenceTransfer();
            return;
        }
    }
}

void nds::Bus7::clock()
{

    if(watchBreakpoints){
        if(std::find(breakpoints.begin(), breakpoints.end(), cpu.getRegisterState().R[15] - (cpu.state() == gba::CpuState::ARM ? 8 : 4)) != breakpoints.end()){
            setHalt();
            return;
        }
        if(std::find(breakpointsOP.begin(), breakpointsOP.end(), cpu.getCurrentOpcode()) != breakpointsOP.end()){
            setHalt();
            return;
        }
    }

    clocks++;
    if(!cpu.haltMode || interruptPending){
        cpu.clock();
    }
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
    setInterruptPending();
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

std::vector<uint64_t> nds::Bus7::addBreakpoint(uint64_t bp)
{
    if(!watchBreakpoints){
        watchBreakpoints = true;
    }

    bool exists = false;

    for(const auto &e : breakpoints){
        if(e==bp)
            exists = true;
    }

    if(!exists){
        breakpoints.push_back(bp);
    }

    return breakpoints;
}

std::vector<uint64_t> nds::Bus7::removeBreakpoint(uint64_t bp)
{
    std::erase_if(breakpoints, [&](uint64_t i) { return i == bp; });


    if(watchBreakpoints && breakpointsOP.size() == 0 && breakpoints.size() == 0){
        watchBreakpoints = false;
    }
    return breakpoints;
}

std::vector<std::string> nds::Bus7::removeBreakpointOP(std::string bp)
{
    std::erase_if(breakpointsOP, [&](std::string i) { return i == bp; });


    if(watchBreakpoints && breakpointsOP.size() == 0 && breakpoints.size() == 0){
        watchBreakpoints = false;
    }
    return breakpointsOP;
}

std::vector<std::string> nds::Bus7::addBreakpointOP(std::string bp)
{
    if(!watchBreakpoints){
        watchBreakpoints = true;
    }

    bool exists = false;

    for(const auto &e : breakpointsOP){
        if(e==bp)
            exists = true;
    }

    if(!exists){
        breakpointsOP.push_back(bp);
    }

    return breakpointsOP;
}
