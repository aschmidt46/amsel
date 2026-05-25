#include "arm7tdmi.h"

using namespace gba;

Byte gba::CPU::readByte(Word addr)
{
    return this->bus.lock()->readByte(addr);
}

HalfWord gba::CPU::readHalfWord(Word addr)
{
    return this->bus.lock()->readHalfWord(addr);
}

Word gba::CPU::readWord(Word addr)
{
    return this->bus.lock()->readWord(addr);
}

void gba::CPU::writeByte(Word addr, Byte val)
{
    this->bus.lock()->writeByte(addr, val);
}

void gba::CPU::writeHalfWord(Word addr, HalfWord val)
{
    this->bus.lock()->writeHalfWord(addr, val);
}

void gba::CPU::writeWord(Word addr, Word val)
{
    this->bus.lock()->writeWord(addr, val);
}
