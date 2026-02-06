#include "apu_square_channel.h"
#include <iostream>

SquareChannel::SquareChannel(bool isSquare2)
{
    this->channel2 = isSquare2;
    sweep = Sweep(channel2);
}

void SquareChannel::setDutyCycle(int c)
{
    switch(c){
        case 0:
            sequencer.setSequenceWithoutReset(sequence0);
            break;
        case 1:
            sequencer.setSequenceWithoutReset(sequence1);
            break;
        case 2:
            sequencer.setSequenceWithoutReset(sequence2);
            break;
        default:
            sequencer.setSequenceWithoutReset(sequence3);
            break;
    }
}

int SquareChannel::getPeriod()
{
    // if(((int)reg3 || ((int)(reg4 & 0b00000111) << 8u) > 0))
    // std::cout << (int)(reg3) << ", " << ((int)(reg4 & 0b00000111) << 8u) << std::endl;
    return (int)(reg3) + ((int)(reg4 & 0b00000111) << 8u);
}

int SquareChannel::getLengthIndex()
{
    return (reg4 & 0b11111000) >> 3;
}

int SquareChannel::shiftRawTimerPeriod(int shift)
{
    return getPeriod() >> shift;
}

int SquareChannel::getDAC()
{
    // if(timer.period >= 8 && length.isPlaying() && envelope.getVolume() > 0)
    //     std::cout << "Volume da" << std::endl;
    return envelope.getVolume() * sweep.isNotMute(this) * (lastSequencerValue > 0) * length.isPlaying() * (timer.period >= 8);
}

void SquareChannel::writeRegister1(uint8_t val)
{
    envelope.onControl(val);
    length.setHaltFlag(val & 0b00100000);
    setDutyCycle((val & 0b11000000) >> 6);
}

void SquareChannel::writeRegister2(uint8_t val)
{
    sweep.onWrite(val, this);
}

void SquareChannel::writeRegister3(uint8_t val)
{
    reg3 = val;
    timer.changePeriod(getPeriod());
}

void SquareChannel::writeRegister4(uint8_t val)
{
    reg4 = val;
    //length.reloadCounter((val & 0b11111000) >> 3);
    length.writeTo(val);
    timer.changePeriod(getPeriod());
    sequencer.restart();
    envelope.wasWrite = true;
}

void SquareChannel::clockLengthCounter()
{
    length.clock();
}

void SquareChannel::clockEnvelope()
{
    envelope.clock();
}

void SquareChannel::clockSweep()
{
    sweep.clock(this);
}

void SquareChannel::onCPUClock()
{
    bool c = timer.clock();
    if(c)
        lastSequencerValue = sequencer.clock();
}
