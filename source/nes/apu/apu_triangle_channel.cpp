#include "apu_triangle_channel.h"

void TriangleChannel::updateTimer()
{
    int period = (int)regA +  (((int)regB & 0b00000111) << 8u);
    timer.changePeriod(period);
}

void TriangleChannel::updateLength()
{
    length.writeTo(regB);
}

void TriangleChannel::clock()
{
    if(timer.clock()){
        if(linearCtr.counter > 0 && length.isPlaying()){
            lastSequencerValue = sequencer.clock();
        }
    }
}

void TriangleChannel::writeA(uint8_t val)
{
    regA = val;
    updateTimer();
}

void TriangleChannel::writeB(uint8_t val)
{
    regB = val;
    updateTimer();
    updateLength();
}

uint8_t TriangleChannel::getLastSequencerValue()
{
    if(timer.period<=1){
        return 7;
    }
    else return lastSequencerValue;
}
