#pragma once
#include <cstdint>

struct LinearCounter{

    bool controlFlag = false;
    bool haltFlag = false;
    uint8_t reloadValue = 0;
    uint8_t counter = 0;

    // Register 0x4008
    void onWrite(uint8_t val);
    void clock();
    void setHaltFlag();
};
