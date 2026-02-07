#pragma once
#include <cstdint>
#include "apu_divider.h"
#include <vector>

struct DMAReader{
    uint16_t addressCounter;
    uint16_t bytesRemain;
};

struct OutputUnit{
    uint8_t shiftReg = 0;
    uint16_t counter = 0;
    bool silenceFlag;
};

class Apu;

struct DeltaModulationChannel{

    std::vector<int> periodTable{
        0x1AC,
        0x17C,
        0x154,
        0x140,
        0x11E,
        0x0FE,
        0x0E2,
        0x0D6,
        0x0BE,
        0x0A0,
        0x08E,
        0x080,
        0x06A,
        0x054,
        0x048,
        0x036
    };

    uint8_t r4012;
    uint8_t r4013;

    uint8_t dac;

    DMAReader dmaReader;
    // 1 Byte
    uint8_t sampleBuffer = 0;
    OutputUnit output;
    bool interruptFlag;
    bool interruptEnabledFlag;
    bool loopFlag;
    Divider timer;
    // 7 bit
    uint8_t counter = 0;

    void restart();
    void clock(Apu* apu);
    void setPeriod(uint8_t p);
    void setInterruptEnabled(bool i);
    void startNewOutputCycle();
    uint8_t getDAC();
    void setD(bool d);
};
