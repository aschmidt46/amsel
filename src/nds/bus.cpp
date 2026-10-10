#include "bus.h"
#include <iostream>
#include "framework/stringlib.h"
#include <assert.h>

using namespace nds;

void nds::SharedBus::loadCartHLE(const std::vector<uint8_t> &bytes)
{
    if(bytes.size() == 0) return;
    gameCart = bytes;
    nds9romRomStart = *(Word*)(bytes.data() + 0x20);
    // nds9romRomStart &= ~0xFFF; // align 0x1000
    nds9romRamStart = *(Word*)(bytes.data() + 0x28);
    nds9romSize = *(Word*)(bytes.data() + 0x2C);

    nds7romRomStart = *(Word*)(bytes.data() + 0x30);
    nds7romRamStart = *(Word*)(bytes.data() + 0x38);
    nds7romSize = *(Word*)(bytes.data() + 0x3C);
    
    Word entrynds9 = *(Word*)(bytes.data() + 0x24);
    Word entrynds7 = *(Word*)(bytes.data() + 0x34);
    
    bus7->cpu.setPC(entrynds7);
    bus7->cpu.setLR(entrynds7);
    bus7->cpu.setIP(entrynds7);
    bus9->cpu.setPC(entrynds9);
    bus9->cpu.setLR(entrynds9);
    bus9->cpu.setIP(entrynds9);
    
    size_t j = nds9romRamStart;
    for(size_t i = nds9romRomStart; i < nds9romRomStart + nds9romSize; i++){
        bus9->writeByte(j, gameCart[i]);
        j++;
    }

    j = nds7romRamStart;
    for(size_t i = nds7romRomStart; i < nds7romRomStart + nds7romSize; i++){
        bus7->writeByte(j, gameCart[i]);
        j++;
    }
}

Byte nds::SharedBus::readByte(Word addr)
{
    if(addr >= 0x02000000 && addr < 0x03000000){
        // main mem
        addr -= 0x02000000;
        return mainMemory[addr % 0x40'0000];
    }
    return 0;
}

void nds::SharedBus::writeByte(Word addr, Byte val)
{
    if(addr >= 0x02000000 && addr < 0x03000000){
        // main mem
        addr -= 0x02000000;
        mainMemory[addr % 0x40'0000] = val;
    }
}

void nds::SharedBus::PPUEnteredHBlank()
{
}

void nds::SharedBus::PPUEnteredVBlank()
{
}

void nds::SharedBus::PPULeftHBlank()
{
}

void nds::SharedBus::PPULeftVBlank()
{
}

void SharedBus::init()
{
    bus7 = std::make_shared<Bus7>(this);
    bus7->init();
    bus9 = std::make_shared<Bus9>(this);
    bus9->init();
    lcd = std::make_shared<LCD>(this);
    lcd->init();
    ipc = std::make_shared<IPC>(this);
    scheduler = std::make_shared<gba::Scheduler<EventType, SharedBus>>(this);
    scheduler->init();
}

void nds::SharedBus::setIF(int bit, bool val)
{
    bus7->setIF(bit, val);   
    bus9->setIF(bit, val);
}

void nds::SharedBus::setIFSingleBus(int bit, bool val, bool nds9)
{
    if(nds9){
        bus9->setIF(bit, val);
    }
    else{
        bus7->setIF(bit, val);
    }
}

void nds::SharedBus::setHalt()
{
    halted = true;
}

uint32_t *nds::SharedBus::accessFramebuffer()
{
    return lcd->accessFramebuffer();
}

std::pair<float, float> nds::SharedBus::getSample()
{
    return {0,0};
}

void nds::SharedBus::clock()
{
    // if(watchBreakpoints){
    //     if(std::find(breakpoints.begin(), breakpoints.end(), cpu.getRegisterState().R[15] - (cpu.state() == ARM ? 8 : 4)) != breakpoints.end()){
    //         halted = true;
    //     }
    //     if(std::find(breakpointsOP.begin(), breakpointsOP.end(), cpu.getCurrentOpcode()) != breakpointsOP.end()){
    //         halted = true;
    //     }
    // }

    if(!halted || steps > 0){
        bus7->clock();
        bus9->clock();
        scheduler->clock();

        if(steps > 0)
            steps--;
    }

}

void nds::SharedBus::press(int i){
    bus9->KEYINPUT.raw &= ~(1u << i);
    if(bus9->KEYCNT.raw & (1u << 14)){ // IRQ enable
        if(bus9->KEYCNT.raw & (1u << i)){
            if(bus9->KEYCNT.raw & (1u << 15)){ // AND
                if((bus9->KEYINPUT.raw & 0b1111111111u) == 0){
                    setIF(12, true);
                }
            }
            else{
                setIF(12, true);
            }
        }
    }
}

void nds::SharedBus::release(int i){
    bus9->KEYINPUT.raw |= (1u << i);
}
