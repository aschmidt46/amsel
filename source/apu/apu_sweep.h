#pragma once
#include "apu_divider.h"
#include <cstdint>
#include <algorithm>

class SquareChannel;

struct Sweep{
    Divider divider;
    bool square2;
    uint8_t raw;
    bool wasWrite = false;

    Sweep(bool isSquare2);

    bool isEnabled();
    bool getNegate();
    uint8_t getP(); // Halbframes
    uint8_t getShift();

    int getTargetPeriod(SquareChannel* ch);

    bool isNotMute(SquareChannel* ch);

    
    // eppp nsss       enable, period, negate, shift
    void onWrite(uint8_t val, SquareChannel* ch); // Zweites Register (4001)

    void clock(SquareChannel* ch);
};
