#pragma once
#include <cstdint>

constexpr const uint8_t lengthTable[32] = {
    10,   //(00)
    254,  //(01)
    20,   //(02)
    2,    //(03)
    40,   //(04)
    4,    //(05)
    80,   //(06)
    6,    //(07)
    16,   //(08)
    8,    //(09)
    60,   //(0A)
    10,   //(0B)
    14,   //(0C)
    12,   //(0D)
    26,   //(0E)
    14,   //(0F)
    12,   //(10)
    16,   //(11)
    24,   //(12)
    18,   //(13)
    48,   //(14)
    20,   //(15)
    96,   //(16)
    22,   //(17)
    19,   //(18)
    24,   //(19)
    72,   //(1A)
    26,   //(1B)
    16,   //(1C)
    28,   //(1D)
    32,   //(1E)
    30    //(1F)
};

struct LengthCounter{
    bool haltFlag;
    unsigned int counter;
    void clear();
    void clock();
    void setHaltFlag(bool h);
    void setEnableFlag(bool e);
    void reloadCounter(uint8_t val);
    bool isPlaying();
    void writeTo(uint8_t val);
};
