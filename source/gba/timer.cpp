#include "timer.h"
#include "bus.h"
#include <iostream>

void gba::Timer::onWrite(gba::Word addr, gba::Byte val){
    if(addr < 2){
        reload.OnWriteByte(addr, val);
    }
    else if(addr < 4){
        bool startBitWasSet = control.raw & (1u << 7);
        control.OnWriteByte(addr,val);
        if(!startBitWasSet && (control.raw & (1u << 7))){
            value = reload.raw;
            dividerValue = 0;
        }
    }
}

gba::Byte gba::Timer::onRead(gba::Word addr){
    if(addr == 0){
        return value & 0xFF;
    }
    else if(addr == 1){
        return value >> 8;
    }
    else if(addr < 4){
        return control.OnReadByte(addr);
    }
    return 0;
}

bool gba::Timer::usesPreviousTimer()
{
    return (control.raw & 4u) && number > 0; // Geht nur, wenn das nicht der erste Timer (t0) ist
}

// 16.78 MHz
void gba::Timer::clock() {
    overflow = false;
    if(control.raw & 128){ // Start bit
        Word divider = control.raw & 0b11;
        this->dividerValue++;
        if(dividerValue >= timerDividers[divider]){
            this->dividerValue = 0;
            onIncrement();
        }
    }
}

void gba::Timer::clockWithPrevious() {
    overflow = false;
    if(control.raw & 128){
        onIncrement();
    }
}

bool gba::Timer::justOverflowed()
{
    bool tmp = overflow;
    overflow = false;
    return tmp;
}

void gba::Timer::onIncrement() {
    this->value++;
    if(this->value > 0xFFFF){ // Overflow
        // std::cout << "Timer " << number << " overflowed!\n";
        overflow = true;
        this->value = reload.raw;
        if(control.raw & 64){ // IRQ Enable
            bus->setIF(3 + number, true); // Interrupt Flag für Timer 0 startet bei bit 3
        }
    }
}
