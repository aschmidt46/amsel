#include "bus9.h"
#include "bus.h"

using namespace gba;

void nds::Bus9::writeByte(Word addr, Byte val)
{
}

Byte nds::Bus9::readByte(Word addr)
{
    return Byte();
}

void nds::Bus9::writeHalfWord(Word addr, HalfWord val)
{
}

HalfWord nds::Bus9::readHalfWord(Word addr)
{
    return HalfWord();
}

void nds::Bus9::writeWord(Word addr, Word val)
{
}

Word nds::Bus9::readWord(Word addr)
{
    return Word();
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

Word nds::Bus9::getIE()
{
    return sharedBus->getIE();
}

Word nds::Bus9::getIF()
{
    return sharedBus->getIF();
}

bool nds::Bus9::hasIME()
{
    return sharedBus->hasIME();
}

void nds::Bus9::setHalt()
{
    sharedBus->setHalt();
}
