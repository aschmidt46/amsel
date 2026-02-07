#pragma once
#include <vector>
#include "apu_divider.h"
#include "apu_linear_counter.h"
#include "apu_length_counter.h"
#include "apu_sequencer.h"

struct TriangleChannel{
    Divider timer;
    LinearCounter linearCtr;
    LengthCounter length;
    Sequencer sequencer{std::vector<uint8_t>{0xF, 0xE, 0xD, 0xC, 0xB, 0xA, 0x9, 0x8, 0x7, 0x6, 0x5, 0x4, 0x3, 0x2, 0x1, 0x0, 0x0, 0x1, 0x2, 0x3, 0x4, 0x5, 0x6, 0x7, 0x8, 0x9, 0xA, 0xB, 0xC, 0xD, 0xE, 0xF}};

    uint8_t lastSequencerValue = 0;

    uint8_t regA = 0, regB = 0;

    void updateTimer();
    void updateLength();

    void clock();
    void writeA(uint8_t val);
    void writeB(uint8_t val);
};
