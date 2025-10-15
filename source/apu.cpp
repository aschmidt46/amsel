#include "apu.h"

void Apu::write(uint16_t reg, uint8_t val)
{
    switch(reg){
        case 0x4000:
            switch((val & 0xC0) >> 6){
                case 0x00: pulse1seq.sequence = 0b00000001; pulse1osc.dutycycle = 0.125; break;
                case 0x01: pulse1seq.sequence = 0b00000011; pulse1osc.dutycycle = 0.250; break;
                case 0x02: pulse1seq.sequence = 0b00001111; pulse1osc.dutycycle = 0.500; break;
                case 0x03: pulse1seq.sequence = 0b11111100; pulse1osc.dutycycle = 0.750; break;
            }
            pulse1seq.sequence = pulse1seq.new_sequence;
            break;
        case 0x4001:
		    break;
	    case 0x4002:
		    pulse1seq.reload = (pulse1seq.reload & 0xFF00) | val;
		    break;
	    case 0x4003:
		    pulse1seq.reload = (uint16_t)((val & 0x07)) << 8 | (pulse1seq.reload & 0x00FF);
		    pulse1seq.timer = pulse1seq.reload;
		    pulse1seq.sequence = pulse1seq.new_sequence;
		    break;
        case 0x4004:
            switch((val & 0xC0) >> 6){
                case 0x00: pulse2seq.sequence = 0b00000001; pulse2osc.dutycycle = 0.125; break;
                case 0x01: pulse2seq.sequence = 0b00000011; pulse2osc.dutycycle = 0.250; break;
                case 0x02: pulse2seq.sequence = 0b00001111; pulse2osc.dutycycle = 0.500; break;
                case 0x03: pulse2seq.sequence = 0b11111100; pulse2osc.dutycycle = 0.750; break;
            }
            pulse2seq.sequence = pulse2seq.new_sequence;
            break;
        case 0x4005:
		    break;
	    case 0x4006:
		    pulse2seq.reload = (pulse2seq.reload & 0xFF00) | val;
		    break;
	    case 0x4007:
		    pulse2seq.reload = (uint16_t)((val & 0x07)) << 8 | (pulse2seq.reload & 0x00FF);
		    pulse2seq.timer = pulse2seq.reload;
		    pulse2seq.sequence = pulse2seq.new_sequence;
		    break;
        break;
        case 0x4008:
        break;
        case 0x4009:
        break;
        case 0x400A:
        break;
        case 0x400B:
        break;
        case 0x400C:
        break;
        case 0x400D:
        break;
        case 0x400E:
        break;
        case 0x400F:
            break;
        case 0x4010:
        break;
        case 0x4011:
        break;
        case 0x4012:
        break;
        case 0x4013:
        break;
        case 0x4015:
		    enablePulse1 = val & 0x01;
		    enablePulse2 = val & 0x02;
		break;
	    case 0x4017:
	    	break;
        default:
        break;
    }
}

void Apu::clock()
{
    bool quarterFrameClock;
    bool halfFrameClock;

    globalTime += (0.3333333333 / 1789773.0);

    if(clockCounter % 6 == 0){
        frameClockCounter++;

        // 4-Step Sequence Mode
        if(frameClockCounter == 3729){
            quarterFrameClock = true;
        }
        if(frameClockCounter == 7457){
            quarterFrameClock = true;
            halfFrameClock = true;
        }
        if(frameClockCounter == 11186){
            quarterFrameClock = true;
        }
        if(frameClockCounter == 14916){
            quarterFrameClock = true;
            halfFrameClock = true;
            frameClockCounter = 0;
        }

        // Quarter frame beats adjust the volume envelope
        if(quarterFrameClock){
        }

        // half frame beats adjust note length and frequency sweepers
        if(halfFrameClock){
        }

			// Update Pulse1 Channel ================================
			pulse1seq.clock(enablePulse1, [](uint32_t &s)
			{
				// Shift right by 1 bit, wrapping around
				s = ((s & 0x0001) << 7) | ((s & 0x00FE) >> 1);
			});
			pulse1osc.frequency = 1789773.0 / (16.0 * (double)(pulse1seq.reload + 1));
			pulse1Sample = pulse1osc.sample(globalTime);

            // Update Pulse2 Channel ================================
			pulse2seq.clock(enablePulse2, [](uint32_t &s)
			{
				// Shift right by 1 bit, wrapping around
				s = ((s & 0x0001) << 7) | ((s & 0x00FE) >> 1);
			});
			pulse2osc.frequency = 1789773.0 / (16.0 * (double)(pulse2seq.reload + 1));
			pulse2Sample = pulse2osc.sample(globalTime);
    }

    clockCounter++;
}

double Apu::getSample()
{
    return pulse1Sample * 0.2 + pulse2Sample * 0.5;
}
