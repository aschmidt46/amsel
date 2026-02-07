#pragma once
#include "apu_divider.h"
#include <cstdint>

struct Envelope{
    bool loopFlag = false;
    unsigned int decayLevelCounter = 0;
    bool disable = false;
    bool wasWrite = false;

    Divider divider;

    // Erstes Kanalregister (4000)
    void onControl(uint8_t val);

    unsigned int getVolume();

    void clock();
};
