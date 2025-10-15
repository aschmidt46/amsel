#pragma once
#include <cstdint>
#include <functional>

constexpr const unsigned int sampleRate = 20000;

class Apu{
    private:
    struct sequencer{
        uint32_t sequence = 0x00000000;
        uint16_t timer = 0x0000;
        uint16_t reload = 0x0000;
        uint8_t output = 0x00;

        uint8_t clock(bool enable, std::function<void(uint32_t &s)> funcManip){
            if(enable){
                timer--;
                if(timer == 0xFFFF){
                    timer = reload + 1;
                    funcManip(sequence);
                    output = sequence & 0x00000001;
                }
            }

            return output;
        };
    };

    struct oscpulse
	{
		double frequency = 0;
		double dutycycle = 0;
		double amplitude = 1;
		double pi = 3.14159;
		double harmonics = 20;

		double sample(double t)
		{
			double a = 0;
			double b = 0;
			double p = dutycycle * 2.0 * pi;

			auto approxsin = [](double t)
			{
				double j = t * 0.15915;
				j = j - (int)j;
				return 20.785 * j * (j - 0.5) * (j - 1.0);
			};

			for (double n = 1; n < harmonics; n++)
			{
				double c = n * frequency * 2.0 * pi * t;
				a += -approxsin(c) / n;
				b += -approxsin(c - p * n) / n;

				//a += -sin(c) / n;
				//b += -sin(c - p * n) / n;
			}

			return (2.0 * amplitude / pi) * (a - b);
		}
	};

    unsigned int clockCounter = 0;
    unsigned int frameClockCounter = 0;

    public:
    void write(uint16_t reg, uint8_t val);
    void clock();

    double getSample();
    double currentSample = 0.0;
    double globalTime = 0.0;

    sequencer pulse1seq;
    oscpulse pulse1osc;
    bool enablePulse1 = false;
    double pulse1Sample = 0.0;
};
