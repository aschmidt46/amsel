#include "apu_frame_sequencer.h"
#include "../6502.h"
#include "apu.h"


FrameSequencer::FrameSequencer()
{
}

void FrameSequencer::onWrite(uint8_t val, Apu* apu)
{
    if(divider.counter==1){ // Clock diesen Zyklus
        resetCounter = 3;
    }
    else resetCounter = 4;

    sequencer0.restart();
    sequencer1.restart();

    mode = val & 0b10000000;
    bool r = val & 0b01000000;
    if(r)
        interruptFlag = false;
    inhibitFlag  = r;
    if(mode){ // https://github.com/100thCoin/AccuracyCoin/README.md
        sequencer1.clock();
        clockLengthCountersAndSweepUnits(apu);
        clockEnvelopesAndTrianglesLinearCounter(apu);
    }
}

void FrameSequencer::clockLengthCountersAndSweepUnits(Apu* apu)
{
    apu->pulse1.clockLengthCounter();
    apu->pulse1.clockSweep();

    apu->pulse2.clockLengthCounter();
    apu->pulse2.clockSweep();

    apu->triangle.length.clock();

    apu->noise.length.clock();
}

void FrameSequencer::clockEnvelopesAndTrianglesLinearCounter(Apu* apu)
{
    apu->pulse1.clockEnvelope();
    apu->pulse2.clockEnvelope();

    apu->triangle.linearCtr.clock();

    apu->noise.envelope.clock();
}

void FrameSequencer::clock(Apu* apu)
{
    if(divider.clock()){
        if(mode) {
            switch(sequencer1.clock()){
                case 1:
                    clockLengthCountersAndSweepUnits(apu);
                    clockEnvelopesAndTrianglesLinearCounter(apu);
                    break;
                case 2:
                    clockEnvelopesAndTrianglesLinearCounter(apu);
                    break;
                case 3:
                    clockLengthCountersAndSweepUnits(apu);
                    clockEnvelopesAndTrianglesLinearCounter(apu);
                    break;
                case 4:
                    clockEnvelopesAndTrianglesLinearCounter(apu);
                    break;
                default:
                    break;
            }
        }
        else {
            switch(sequencer0.clock()){
                case 1:
                    clockEnvelopesAndTrianglesLinearCounter(apu);
                    break;
                case 2:
                    clockLengthCountersAndSweepUnits(apu);
                    clockEnvelopesAndTrianglesLinearCounter(apu);
                    break;
                case 3:
                    clockEnvelopesAndTrianglesLinearCounter(apu);
                    break;
                default:
                    clockLengthCountersAndSweepUnits(apu);
                    clockEnvelopesAndTrianglesLinearCounter(apu);
                    if(!inhibitFlag){
                        interruptFlag = true;

                        /*
                            "Since Frame IRQs are a worthless misfeature of the NES anyway, anything that suppresses them will improve game compatibility.
                            Just a few games actually use the feature, such as the JP version of Dragon Quest 1, 2, and Door Door.
                            (This also has the side effect of making those games play music at the correct speed on Dendy)

                            Frame IRQs were useful for the arcade games which used the 2A03 without a PPU, Punch Out and Donkey Kong 3."

                            https://forums.nesdev.org/viewtopic.php?t=24582
                        */

                        // Das Timing des Frame Sequencers ist leider kaputt, daher werden die Interrupts manche Spiele zum Crash bringen
                        // apu->mapper->pullIRQ();
                    }
                    break;
            }
        }
    }

    // APU Referenz:
    // "At any time if the interrupt flag is set and the IRQ disable is clear, the CPU's IRQ line is asserted."
    // if(interruptFlag && !inhibitFlag){
    //     apu->mapper->pullIRQ();
    // }
}
