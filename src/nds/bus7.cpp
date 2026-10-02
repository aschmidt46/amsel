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

Word nds::Bus7::getIE()
{
    return sharedBus->getIE();
}

Word nds::Bus7::getIF()
{
    return sharedBus->getIF();
}

bool nds::Bus7::hasIME()
{
    return sharedBus->hasIME();
}

void nds::Bus7::setHalt()
{
    sharedBus->setHalt();
}
