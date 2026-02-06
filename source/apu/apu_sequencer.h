#pragma once
#include <vector>
#include <cstdint>

struct Sequencer{
    std::vector<uint8_t> sequence;
    unsigned int counter = 0;

    // Sequencer() = default;
    Sequencer(const std::vector<uint8_t> &v);

    void setSequenceWithoutReset(const std::vector<uint8_t> &v);

    void restart();

    uint8_t clock();
};
