#include "apu_noise_channel.h"

void NoiseChannel::onWrite(uint8_t val)
{
    timer.changePeriod(periodTable[val & 0b00001111]);
    mode = val & 0b10000000;
}

void NoiseChannel::clock()
{
    if(timer.clock()){
        // bit 6 bzw. bit 1
        bool secondBit = mode ? shiftRegister & 0x40 : shiftRegister & 0b10;
        bool resultBit = (shiftRegister & 1) != secondBit;
        shiftRegister = (shiftRegister >> 1) | (resultBit << 14);
    }
}

uint8_t NoiseChannel::getDAC()
{
    if((shiftRegister & 1) && length.isPlaying()){
        return envelope.getVolume();
    }
    else return 0;
}
