#include "apu_length_counter.h"

void LengthCounter::clear()
{
    haltFlag = true;
    counter = 0;
}

void LengthCounter::clock()
{
    if(haltFlag) return;
    
    if(counter > 0)
        counter--;
}

void LengthCounter::setHaltFlag(bool h)
{
    haltFlag = h;
}

// Quasi enable = !halt ? 
void LengthCounter::setEnableFlag(bool e)
{
    if(e) return;
    else clear();
}

void LengthCounter::reloadCounter(uint8_t val)
{
    if(!haltFlag){
        counter = val;
    }
}

bool LengthCounter::isPlaying()
{
    if(counter==0)
        counter = 0;
    return counter > 0;
}

void LengthCounter::writeTo(uint8_t val)
{
    if(!haltFlag){
        uint8_t bits = (val & 0b11111000) >> 3;
        counter = lengthTable[bits];
    }
}
