#include "apu_sweep.h"
#include "apu_square_channel.h"

Sweep::Sweep(bool isSquare2)
{
    square2 = isSquare2;
}

bool Sweep::isEnabled()
{
    return (raw & 0b10000000) && getShift() > 0;
}

bool Sweep::getNegate()
{
    return raw & 0b00001000;
}

uint8_t Sweep::getP()
{
    return ((raw & 0b01110000) >> 4) + 1;
}

uint8_t Sweep::getShift()
{
    return raw & 0b00000111;
}

int Sweep::getTargetPeriod(SquareChannel *ch) //"clock"?
{
    int changeAmount = ch->timer.period >> (unsigned int)getShift();

    int c = square2 ? 0 : 1;

    if (getNegate()){
        changeAmount = 0 - changeAmount - c;
    }

    return std::max((int)ch->timer.period + changeAmount, 0);
}

bool Sweep::isNotMute(SquareChannel *ch)
{
    int p = getTargetPeriod(ch); 
    return p <= 0x7FF && ch->timer.period >= 8;
}

void Sweep::onWrite(uint8_t val, SquareChannel* ch)
{
    (void)ch;
    raw = val;
    divider.changePeriod(getP());
    wasWrite = true;
    //ch->updatePeriod(getTargetPeriod(ch));
}

void Sweep::clock(SquareChannel *ch)
{
    if(divider.clock()){
        if(isEnabled() && isNotMute(ch)){
            ch->updatePeriod(getTargetPeriod(ch));
        }
    }

    if(wasWrite){
        wasWrite = false;
        divider.reset();
    }
}
