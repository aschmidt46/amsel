#pragma once
#include "apu_envelope.h"
#include "apu_sweep.h"
#include "apu_divider.h"
#include "apu_sequencer.h"
#include "apu_length_counter.h"


const std::vector<uint8_t> sequence0{0,1,0,0,0,0,0,0};
const std::vector<uint8_t> sequence1{0,1,1,0,0,0,0,0};
const std::vector<uint8_t> sequence2{0,1,1,1,1,0,0,0};
const std::vector<uint8_t> sequence3{1,0,0,1,1,1,1,1};

const std::vector<std::vector<uint8_t>> sequences{sequence0, sequence1, sequence2, sequence3};

struct SquareChannel{
    bool channel2 = false;
    Envelope envelope;
    Sweep sweep = Sweep(false);
    Divider timer = Divider(0,2);
    Sequencer sequencer = Sequencer(sequence0);
    LengthCounter length;

    uint8_t lastSequencerValue = 0;


    //$4000/4 ddle nnnn   duty, loop env/disable length, env disable, vol/env
    // period
    // $4001/5 eppp nsss   enable sweep, period, negative, shift (Sweep intern?)
    // $4002/6 pppp pppp   period low
    // $4003/7 llll lppp   length index, period high

    uint8_t reg3 = 0;
    uint8_t reg4 = 0;

    SquareChannel(bool isSquare2);

    void setDutyCycle(int c);
    
    int calculatePeriodfromRegisters();
    void updatePeriod(uint16_t p);

    int getLengthIndex();

    int getDAC();
    void writeRegister1(uint8_t val); // 0x4000, 0x4004
    void writeRegister2(uint8_t val); // 0x4001, 0x4006
    void writeRegister3(uint8_t val); // 0x4002, 0x4006
    void writeRegister4(uint8_t val); // 0x4003, 0x4007

    void clockLengthCounter();
    void clockEnvelope();
    void clockSweep();

    void onCPUClock();
};