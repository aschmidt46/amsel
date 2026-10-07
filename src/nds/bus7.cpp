#include "bus7.h"
#include "bus.h"

using namespace nds;

void nds::Bus7::init()
{
    new (&cpu) gba::CPU(this, false);
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
    else if(addr >= 0x4000180 && addr < 0x4000188){
        sharedBus->ipc->writeReg(addr, val, false);
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
    if(addr >= 0x4000180 && addr < 0x4000188){
        return sharedBus->ipc->readReg(addr, false);
    }

    if(addr == 0x4000241){
        return sharedBus->wramCNT;
    }

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

void nds::Bus7::clock()
{
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
