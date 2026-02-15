#include "apu_linear_counter.h"

void LinearCounter::onWrite(uint8_t val)
{
    controlFlag = val & 0b10000000;
    reloadValue = val & 0b01111111;
}

void LinearCounter::clock()
{
    if(haltFlag)
        counter = reloadValue;
    else if (counter > 0) counter--;

    if(!controlFlag) haltFlag = false;
}

void LinearCounter::setHaltFlag()
{
    haltFlag = true;
}
