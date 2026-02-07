#pragma once
#include <cstdint>

struct LinearCounter{

    bool controlFlag;
    bool haltFlag;
    uint8_t reloadValue;
    uint8_t counter;

    // Register 0x4008
    void onWrite(uint8_t val);
    void clock();
    void setHaltFlag();
};
