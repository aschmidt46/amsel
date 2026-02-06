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

bool LengthCounter::isPlaying()
{
    return counter > 0;
}

// reload
void LengthCounter::writeTo(uint8_t val)
{
    if(!haltFlag){
        int bits = (val & 0b11111000) >> 3;
        counter = lengthTable[bits];
    }
}
