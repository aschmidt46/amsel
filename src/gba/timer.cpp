#include "timer.h"
#include "bus.h"
#include <iostream>
#include <bitset>

void gba::Timer::onWrite(gba::Word addr, gba::Byte val){
    if(addr < 2){
        reload.OnWriteByte(addr, val);
    }
    else if(addr < 4){
        bool startBitWasSet = control.raw & (1u << 7);
        control.OnWriteByte(addr,val);
        if(!startBitWasSet && (control.raw & (1u << 7))){
            value = reload.raw;
            if(!usesPreviousTimer()){
                reschedule();
            }
        }
    }
}

gba::Byte gba::Timer::onRead(gba::Word addr){
    const size_t clocksPassed = bus->getClocks() - clocksStart;
    const size_t valueIncrement = clocksPassed / timerDividers[control.raw & 0b11];
    if(addr == 0){
        if(usesPreviousTimer())
            return value & 0xFF;
        else
            return (value + valueIncrement) & 0xFF;
    }
    else if(addr == 1){
        if(usesPreviousTimer())
            return value >> 8;
        else
            return (value + valueIncrement) >> 8;
    }
    else if(addr < 4){
        return control.OnReadByte(addr);
    }
    return 0;
}

void gba::Timer::reschedule()
{
    Word divider = control.raw & 0b11;
    numStart++;
    clocksStart = bus->getClocks();
    bus->scheduler->scheduleEvent({.timePoint = timerDividers[divider] * (0xFFFF - reload.raw), .type = EVENT_TimerOverflow, .args = {.index = number}});
}

void gba::Timer::overflowTimer()
{
    if(!usesPreviousTimer()){
        const size_t clocksPassed = bus->getClocks() - clocksStart;
        const size_t valueIncrement = clocksPassed / timerDividers[control.raw & 0b11];
        if(value + valueIncrement < 0xFFFF) // Timer wurde später erneut gesetzt
            return;
    }
    if(control.raw & 128){ // sicherstellen, dass dieses Timerevent vom letzten Stellen des Timers kam
        this->value = reload.raw;
        if(control.raw & 64){ // IRQ Enable
            bus->setIF(3 + number, true); // Interrupt Flag für Timer 0 startet bei bit 3
        }
        bus->timerOverflowed(number);
        if(!usesPreviousTimer()){
            reschedule();
        }
    }
}

bool gba::Timer::usesPreviousTimer()
{
    return (control.raw & 4u) && number > 0; // Geht nur, wenn das nicht der erste Timer (t0) ist
}

void gba::Timer::clockWithPrevious() {
    if(control.raw & 128){
        onIncrement();
    }
}

void gba::Timer::onIncrement() {
    this->value++;
    if(this->value > 0xFFFF){ // Overflow
        overflowTimer();
    }
}
