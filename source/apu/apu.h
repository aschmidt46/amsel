#pragma once
#include <cstdint>
#include <functional>
#include "apu_frame_sequencer.h"
#include "apu_square_channel.h"


class Apu{
    private:
	Cpu* cpu;
	uint8_t status; // 0x4015

    public:
	Apu(Cpu* cpu);
    void write(uint16_t reg, uint8_t val);
	uint8_t read(uint16_t reg); // Nur status
    void clock();

    double getSample(bool s);

	double pulse1Sample = 0.0;

	double pulse2Sample = 0.0;

	FrameSequencer fseq = FrameSequencer(this);

	SquareChannel pulse1 = SquareChannel(false);
	SquareChannel pulse2 = SquareChannel(true);
};
