#include "bus7.h"
#include "bus.h"

using namespace nds;

void nds::Bus7::writeByte(Word addr, Byte val)
{
}

Byte nds::Bus7::readByte(Word addr)
{
    return Byte();
}

void nds::Bus7::writeHalfWord(Word addr, HalfWord val)
{
}

HalfWord nds::Bus7::readHalfWord(Word addr)
{
    return HalfWord();
}

void nds::Bus7::writeWord(Word addr, Word val)
{
}

Word nds::Bus7::readWord(Word addr)
{
    return Word();
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
