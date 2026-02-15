#pragma once
#include "apu_divider.h"
#include "apu_sequencer.h"

class Apu;


struct FrameSequencer{
    Divider divider = Divider(7458);
    Sequencer sequencer0 = Sequencer({1, 2, 3, 4});
    Sequencer sequencer1 = Sequencer({1, 2, 3, 4, 5});

    Apu* apu;
    int resetCounter = -1;

    FrameSequencer(Apu* apu);

    bool mode = false; // false 0, true 1

    bool interruptFlag = false; // BRAUCHT IMPLEMENTIERUNG IN APU
    bool inhibitFlag = false;

    void onWrite(uint8_t val);  //0x4017

    void clockLengthCountersAndSweepUnits();
    void clockEnvelopesAndTrianglesLinearCounter();

    void clock();
};
