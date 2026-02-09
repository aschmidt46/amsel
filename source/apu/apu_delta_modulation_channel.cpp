#include "apu_delta_modulation_channel.h"
#include "apu.h"
#include "../mapper.h"

void DeltaModulationChannel::restart()
{
    dmaReader = {.addressCounter = (uint16_t)(r4012 * 0x40u + 0xC000u), .bytesRemain = (uint16_t)(r4013 * 0x10u + 1u)};
}

void DeltaModulationChannel::clock(Apu* apu)
{
    if(interruptFlag) apu->mapper->pullIRQ();

    if(!sampleBuffer && dmaReader.bytesRemain > 0){
        sampleBuffer = apu->mapper->read((uint8_t*)(uintptr_t)dmaReader.addressCounter);
        // Probleme bei AccuracyCoin -> Sehr große remainingCycles -> CPU arbeitet garnicht mehr
        //apu->mapper->cpu->waitFor(4);
        if(dmaReader.addressCounter==0xFFFF)
            dmaReader.addressCounter = 0x8000;
        else dmaReader.addressCounter++;

        dmaReader.bytesRemain--;

        if(dmaReader.bytesRemain==0 && loopFlag)
            restart();
        else if(dmaReader.bytesRemain==0 && interruptEnabledFlag){
            interruptFlag = true;
            apu->mapper->pullIRQ();
        }
    }

    if(timer.clock()){
        if(!output.silenceFlag){
            bool bit0 = output.shiftReg & 1;
            if(!bit0){
                if(counter > 1) counter -= 2;
            }
            else{
                if(counter < 126) counter += 2;
            }
        }

        output.shiftReg = output.shiftReg >> 1;

        if(output.counter > 0) output.counter--;
        if(output.counter==0) startNewOutputCycle();
    }
}

void DeltaModulationChannel::setPeriod(uint8_t p)
{
    timer.changePeriod(periodTable[p]);
}

void DeltaModulationChannel::setInterruptEnabled(bool i)
{
    interruptEnabledFlag = i;
    if(!interruptEnabledFlag)
        interruptFlag = false;
}

void DeltaModulationChannel::startNewOutputCycle()
{
    output.counter = 8;
    if(sampleBuffer == 0)
        output.silenceFlag = true;
    else{
        output.silenceFlag = false;
        output.shiftReg = sampleBuffer;
        sampleBuffer = 0;
    }
}

uint8_t DeltaModulationChannel::getDAC()
{
    return counter;
}

// Status Register
void DeltaModulationChannel::setD(bool d)
{
    if(d && !dmaReader.bytesRemain)
        restart();
    if(!d) dmaReader.bytesRemain = 0;
}
