#include "bus7.h"
#include "bus.h"

using namespace nds;

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
    if(addr >= 0x4000180 && addr < 0x4000188){
        sharedBus->ipc->writeReg(addr, val, false);
    }
}

Byte nds::Bus7::readByteFromWide(Word addr)
{
    if(addr >= 0x4000180 && addr < 0x4000188){
        return sharedBus->ipc->readReg(addr, false);
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
