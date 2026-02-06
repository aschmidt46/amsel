#include "apu.h"
#include <iostream>

Apu::Apu(Cpu *cpu)
{
    this->cpu = cpu;
    square_table = new float[31];
    for(int i = 0; i < 31; i++){
        square_table[i] = 95.52 / (8128.0 / i + 100);
    }
}

Apu::~Apu()
{
    delete[] square_table;
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
            fseq.onWrite(val);
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
    return (fseqi << 6) & ((pulse2.length.isPlaying()) << 1) & ((pulse1.length.isPlaying()));
}

void Apu::clock()
{
    fseq.clock(cpu);
    pulse1.onCPUClock();
    pulse2.onCPUClock();

    int p1Sample = pulse1.getDAC();
    int p2Sample = pulse2.getDAC();
    square_sample = square_table[p1Sample + p2Sample];
    // if(pulse1Sample > 0) std::cout << "Pulse1: " << pulse1Sample << std::endl;
    // if(pulse2Sample > 0) std::cout << "Pulse2: " << pulse2Sample << std::endl;
}

double Apu::getSample(bool s)
{
    return s * (square_sample);
}
