#pragma once
#include <cstdint>
#include <functional>
#include "apu_frame_sequencer.h"
#include "apu_square_channel.h"
#include "apu_triangle_channel.h"
#include "apu_noise_channel.h"
#include "apu_delta_modulation_channel.h"
#include <memory>

class Mapper;

class Apu{
	private:
	double square_table[31];
	double tnd_table[203];
	uint8_t status; // 0x4015
	
    public:
	std::shared_ptr<Mapper> mapper;
	Apu();
	~Apu();
    void write(uint16_t reg, uint8_t val);
	uint8_t read(uint16_t reg); // Nur status
    void clock();
	void reset(std::shared_ptr<Mapper> m);

    double getSample(bool s);

	double square_sample = 0.0;

	double tnd_sample = 0.0;

	FrameSequencer fseq = FrameSequencer();

	SquareChannel pulse1 = SquareChannel(false);
	SquareChannel pulse2 = SquareChannel(true);

	TriangleChannel triangle;

	NoiseChannel noise;

	DeltaModulationChannel dmc;
};
