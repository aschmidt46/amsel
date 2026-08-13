#include "apu_square_channel.h"
#include <iostream>

SquareChannel::SquareChannel(bool isSquare2)
{
    this->channel2 = isSquare2;
    sweep = Sweep(channel2);
}

void SquareChannel::setDutyCycle(int c)
{
    sequencer.setSequenceWithoutReset(sequences[c]);
}

int SquareChannel::calculatePeriodfromRegisters()
{
    // if(((int)reg3 || ((int)(reg4 & 0b00000111) << 8u) > 0))
    // std::cout << (int)(reg3) << ", " << ((int)(reg4 & 0b00000111) << 8u) << std::endl;
    return (int)(reg3) | ((int)(reg4 & 0b00000111) << 8u);
}

void SquareChannel::updatePeriod(uint16_t p)
{
    timer.changePeriod(p);
    reg3 = (uint8_t)p & 0b11111111;
    reg4 = (reg4 & 0b11111000) | ((p & 0b11100000000) >> 8u);
}

int SquareChannel::getLengthIndex()
{
    return (reg4 & 0b11111000) >> 3;
}

int SquareChannel::getDAC()
{
    return envelope.getVolume() * sweep.isNotMute(this) * ((bool)(lastSequencerValue > 0)) * length.isPlaying();
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
    updatePeriod(calculatePeriodfromRegisters());
    // updatePeriod(sweep.getTargetPeriod(this));
}

void SquareChannel::writeRegister4(uint8_t val)
{
    reg4 = val;
    length.writeTo(val);
    updatePeriod(calculatePeriodfromRegisters());
    // updatePeriod(sweep.getTargetPeriod(this));
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
    if(timer.clock())
        lastSequencerValue = sequencer.clock();
}
