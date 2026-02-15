#pragma once
#include <cstdint>
#include "apu_divider.h"
#include <vector>

struct DMAReader{
    uint16_t addressCounter = 0;
    uint16_t bytesRemain = 0;
};

struct OutputUnit{
    uint8_t shiftReg = 0;
    uint16_t counter = 0;
    bool silenceFlag = false;
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

    uint8_t r4012 = 0;
    uint8_t r4013 = 0;

    DMAReader dmaReader;
    // 1 Byte
    uint8_t sampleBuffer = 0;
    OutputUnit output;
    bool interruptFlag = false;
    bool interruptEnabledFlag = false;
    bool loopFlag = false;
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
