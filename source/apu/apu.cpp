#include "apu.h"
#include <iostream>

Apu::Apu(Cpu *cpu)
{
    this->cpu = cpu;
}

void Apu::write(uint16_t reg, uint8_t val)
{
    switch(reg){
        case 0x4000:
            pulse1.writeRegister1(val);
            break;
        case 0x4001:
            pulse1.writeRegister2(val);
		    break;
	    case 0x4002:
		    pulse1.writeRegister3(val);
		    break;
	    case 0x4003:
		    pulse1.writeRegister4(val);
		    break;
        case 0x4004:
            pulse2.writeRegister1(val);
            break;
        case 0x4005:
            pulse2.writeRegister2(val);
		    break;
	    case 0x4006:
		    pulse2.writeRegister3(val);
		    break;
	    case 0x4007:
		    pulse2.writeRegister4(val);
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
        case 0x4015: // Setzt length counter ENABLE flags in pulse 1 pulse 2, usw (Status register)
		    pulse1.length.setEnableFlag(val & 0x01);
		    pulse2.length.setEnableFlag(val & 0x02);
		break;
	    case 0x4017:
	    	break;
        default:
        break;
    }
}

uint8_t Apu::read(uint16_t reg)
{
    bool fseqi = fseq.interruptFlag;
    if(reg==0x4015){
        fseq.interruptFlag = false;
    }
    return (fseqi << 6) & ((pulse2.length.counter > 0) << 1) & ((pulse1.length.counter > 0));
}

void Apu::clock()
{
    fseq.clock(cpu);
    pulse1.onCPUClock();
    pulse2.onCPUClock();

    pulse1Sample = (double)pulse1.getDAC() / 15.0; // Envelope zwischen 0 und 15
    pulse2Sample = (double)pulse2.getDAC() / 15.0;
    // if(pulse1Sample > 0) std::cout << "Pulse1: " << pulse1Sample << std::endl;
    // if(pulse2Sample > 0) std::cout << "Pulse2: " << pulse2Sample << std::endl;
}

double Apu::getSample(bool s)
{
    return s * (pulse1Sample * 0.2 + pulse2Sample * 0.5);
}
