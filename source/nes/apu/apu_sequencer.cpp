#include "apu_sequencer.h"
#include <iostream>

Sequencer::Sequencer(const std::vector<uint8_t> &v)
{
    sequence = v;
    if(sequence.size()==0){
        std::cout << "Ungültige Sequenz (leer)" << std::endl;
        throw;
    }
}

void Sequencer::setSequenceWithoutReset(const std::vector<uint8_t> &v)
{
    sequence = v;
    if(sequence.size()==0){
        std::cout << "Ungültige Sequenz (leer)" << std::endl;
        throw;
    }
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
