#include "apu_sweep.h"
#include "apu_square_channel.h"

Sweep::Sweep(bool isSquare2)
{
    square2 = isSquare2;
}

bool Sweep::isEnabled()
{
    return raw & 0b10000000;
}

bool Sweep::getNegate()
{
    return raw & 0b00001000;
}

uint8_t Sweep::getP()
{
    return (raw & 0b01110000 >> 4) + 1;
}

uint8_t Sweep::getShift()
{
    return raw & 0b00000111;
}

int Sweep::getTargetPeriod(SquareChannel *ch) //"clock"?
{
    int changeAmount = ch->shiftRawTimerPeriod(getShift());
    int c = square2 ? 0 : 1;
    if (getNegate())
        changeAmount = -changeAmount - c;
    return std::max((int)ch->timer.period + changeAmount, 0);
}

bool Sweep::isNotMute(SquareChannel *ch)
{
    int p = getTargetPeriod(ch); 
    return p <= 0x7FF;
}

void Sweep::onWrite(uint8_t val, SquareChannel* ch)
{
    raw = val;
    divider.changePeriod(getP());
    wasWrite = true;
    // ch->timer.changePeriod(getTargetPeriod(ch));
}

void Sweep::clock(SquareChannel *ch)
{
    if(divider.counter <= 0 && isEnabled() && getShift() != 0){
        if(isNotMute(ch)){
            ch->timer.changePeriod(getTargetPeriod(ch));
            //divider.reset(); // Das steht so nicht explizit da, macht aber ansonsten keinen Sinn, oder doch?
        }
        else if (!isNotMute(ch)){
            divider.reset();
        }
    }
    if(divider.counter <= 0 || wasWrite){
        divider.reset();
        wasWrite = false;
    }
    else divider.counter--;
}
