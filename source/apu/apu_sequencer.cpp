#include "apu_sequencer.h"

Sequencer::Sequencer(const std::vector<uint8_t> &v)
{
    sequence = v;
    if(sequence.size()==0) throw;
}

void Sequencer::setSequenceWithoutReset(const std::vector<uint8_t> &v)
{
    sequence = v;
    if(sequence.size()==0) throw;
}

void Sequencer::restart()
{
    counter = 0;
}

uint8_t Sequencer::clock()
{
    if(counter>=sequence.size())
        counter = 0;
    uint8_t res = sequence[counter];
    counter++;
    return res;
}
