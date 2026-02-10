#include "apu.h"
#include <iostream>

Apu::Apu()
{
    square_table = new double[31];
    tnd_table = new double[203];
    for(int i = 0; i < 31; i++){
        square_table[i] = 95.52 / (8128.0 / i + 100);
    }
    for(int i = 0; i < 203; i++){
        tnd_table[i] = 163.67 / (24329.0 / i + 100);
    }
}

Apu::~Apu()
{
    delete[] square_table;
    delete[] tnd_table;
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
        case 0x4008:
            triangle.linearCtr.onWrite(val);
            triangle.length.setHaltFlag(val & 0b10000000);
            break;
        case 0x4009:
        break;
        case 0x400A:
            triangle.writeA(val);
            break;
        case 0x400B:
            triangle.writeB(val);
            triangle.linearCtr.setHaltFlag();
            break;
        case 0x400C:
            noise.envelope.onControl(val);
            noise.length.setHaltFlag(val & 0b00100000);
            break;
        case 0x400D:
        break;
        case 0x400E:
            noise.onWrite(val);
            break;
        case 0x400F:
            noise.length.writeTo(val);
            break;
        case 0x4010:
            dmc.setInterruptEnabled(val & 0b10000000);
            dmc.loopFlag = val & 0b01000000;
            dmc.setPeriod(val & 0b00001111);
            break;
        case 0x4011:
            dmc.counter = (val & 0b01111111);
            break;
        case 0x4012:
            dmc.r4012 = val;
            break;
        case 0x4013:
            dmc.r4013 = val;
            break;
        case 0x4015: // Setzt length counter ENABLE flags in pulse 1 pulse 2, usw (Status register)
		    pulse1.length.setEnableFlag(val & 0x01);
		    pulse2.length.setEnableFlag(val & 0x02);
            triangle.length.setEnableFlag(val & 0x04);
		    noise.length.setEnableFlag(val & 0x08);
            dmc.interruptFlag = false;
            dmc.setD(val & 0x10);
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
    return (dmc.interruptFlag << 7)
        | (fseqi << 6)
        | ((dmc.dmaReader.bytesRemain > 0) << 4)
        | ((noise.length.isPlaying()) << 3)
        | ((triangle.length.isPlaying()) << 2)
        | ((pulse2.length.isPlaying()) << 1)
        | ((pulse1.length.isPlaying()));
}

void Apu::clock()
{
    fseq.clock();
    pulse1.onCPUClock();
    pulse2.onCPUClock();
    triangle.clock();
    noise.clock();
    dmc.clock(this);

    int p1Sample = pulse1.getDAC();
    int p2Sample = pulse2.getDAC();
    square_sample = square_table[p1Sample + p2Sample];

    int tSample = triangle.getLastSequencerValue();
    int nSample = noise.getDAC();
    int dmcS = dmc.getDAC();
    tnd_sample = tnd_table[3 * tSample + 2 * nSample + dmcS];
}

void Apu::reset(std::shared_ptr<Mapper> m)
{
    this->mapper = m;
    fseq = FrameSequencer(this);

	pulse1 = SquareChannel(false);
	pulse2 = SquareChannel(true);

	triangle = TriangleChannel();

	noise = NoiseChannel();

	dmc = DeltaModulationChannel();
}

double Apu::getSample(bool s)
{
    return s * (square_sample + tnd_sample);
}
