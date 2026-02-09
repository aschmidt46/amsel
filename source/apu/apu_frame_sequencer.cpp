#include "apu_frame_sequencer.h"
#include "../6502.h"
#include "apu.h"


FrameSequencer::FrameSequencer(Apu *apu)
{
    this->apu = apu;
}

void FrameSequencer::onWrite(uint8_t val)
{
    divider.reset();
    sequencer0.restart();
    sequencer1.restart();
    mode = val & 0b10000000;
    bool r = val & 0b01000000;
    if(r)
        interruptFlag = false;
    inhibitFlag  = r;
    if(mode){ // https://github.com/100thCoin/AccuracyCoin/README.md
        apu->pulse1.clockLengthCounter();
        apu->pulse2.clockLengthCounter();    

        apu->triangle.length.clock();

        apu->noise.length.clock();
    }
}

void FrameSequencer::clockLengthCountersAndSweepUnits()
{
    apu->pulse1.clockLengthCounter();
    apu->pulse1.clockSweep();

    apu->pulse2.clockLengthCounter();
    apu->pulse2.clockSweep();

    apu->triangle.length.clock();

    apu->noise.length.clock();
}

void FrameSequencer::clockEnvelopesAndTrianglesLinearCounter()
{
    apu->pulse1.clockEnvelope();
    apu->pulse2.clockEnvelope();

    apu->triangle.linearCtr.clock();

    apu->noise.envelope.clock();
}

void FrameSequencer::clock()
{
    if(divider.clock()){
        if(mode) {
            switch(sequencer1.clock()){
                case 1:
                    clockLengthCountersAndSweepUnits();
                    clockEnvelopesAndTrianglesLinearCounter();
                    break;
                case 2:
                    clockEnvelopesAndTrianglesLinearCounter();
                    break;
                case 3:
                    clockLengthCountersAndSweepUnits();
                    clockEnvelopesAndTrianglesLinearCounter();
                    break;
                case 4:
                    clockEnvelopesAndTrianglesLinearCounter();
                    break;
                default:
                    break;
            }
        }
        else {
            switch(sequencer0.clock()){
                case 1:
                    clockEnvelopesAndTrianglesLinearCounter();
                    break;
                case 2:
                    clockLengthCountersAndSweepUnits();
                    clockEnvelopesAndTrianglesLinearCounter();
                    break;
                case 3:
                    clockEnvelopesAndTrianglesLinearCounter();
                    break;
                default:
                    clockLengthCountersAndSweepUnits();
                    clockEnvelopesAndTrianglesLinearCounter();
                    if(!inhibitFlag){
                        interruptFlag = true;
                        apu->mapper->pullIRQ();
                    }
                    break;
            }
        }
    }
}
