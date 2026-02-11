#include "apu_envelope.h"
#include <iostream>

void Envelope::onControl(uint8_t val)
{
    divider.changePeriod((val & 0b00001111) + 1);
    // Unmöglich, weil das eine Lautstärke von 16 ergeben könnte, korrektes Verhalten ist mir nicht bekannt...
    if(divider.period==16)
        divider.changePeriod(0);
    loopFlag = (val & 0b00100000) > 0 ? true : false;
    disable = (val & 0b00010000) > 0 ? true : false;
}

unsigned int Envelope::getVolume()
{
    return disable ? divider.period : decayLevelCounter;
}

void Envelope::clock()
{
    if(wasWrite){
        decayLevelCounter = 15;
        divider.reset();
    }
    else{
        if(divider.clock()){
            if(loopFlag && decayLevelCounter <= 0){
                decayLevelCounter = 15;
            }
            else if(decayLevelCounter > 0){
                decayLevelCounter--;
            }
        }
    }
    wasWrite = false;
}
