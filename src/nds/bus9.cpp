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
