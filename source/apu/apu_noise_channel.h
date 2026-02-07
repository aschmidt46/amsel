#pragma once
#include <cstdint>
#include "apu_divider.h"
#include "apu_envelope.h"
#include "apu_length_counter.h"
#include <vector>



struct NoiseChannel{
    std::vector<int> periodTable{
        0x004,
        0x008,
        0x010,
        0x020,
        0x040,
        0x060,
        0x080,
        0x0A0,
        0x0CA,
        0x0FE,
        0x17C,
        0x1FC,
        0x2FA,
        0x3F8,
        0x7F2,
        0xFE4
    };

    Divider timer = Divider(4);
    Envelope envelope;
    LengthCounter length;
    uint16_t shiftRegister = 1;
    bool mode = false;

    void onWrite(uint8_t val);
    void clock();
    uint8_t getDAC();
};
