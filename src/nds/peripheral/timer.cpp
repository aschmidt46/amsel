#include "timer.h"
#include "../bus.h"
#include <iostream>
#include <bitset>
#include "../bus7.h"
#include "../bus9.h"

using namespace nds;

template<typename ConcreteBus>
void Timer<ConcreteBus>::onWrite(Word addr, Byte val){
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

template<typename ConcreteBus>
Byte Timer<ConcreteBus>::onRead(Word addr){
    const size_t clocksPassed = cbus->getClocks() - clocksStart;
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

template<typename ConcreteBus>
void Timer<ConcreteBus>::reschedule()
{
    Word divider = control.raw & 0b11;
    numStart++;
    clocksStart = cbus->getClocks();
    bus->scheduler->scheduleEvent({.timePoint = timerDividers[divider] * (0xFFFF - reload.raw), .type = scheduleEvent, .args = {.index = number}});
}

template<typename ConcreteBus>
void Timer<ConcreteBus>::overflowTimer()
{
    if(!usesPreviousTimer()){
        const size_t clocksPassed = cbus->getClocks() - clocksStart;
        const size_t valueIncrement = clocksPassed / timerDividers[control.raw & 0b11];
        if(value + valueIncrement < 0xFFFF) // Timer wurde später erneut gesetzt
            return;
    }
    if(control.raw & 128){ // sicherstellen, dass dieses Timerevent vom letzten Stellen des Timers kam
        this->value = reload.raw;
        if(control.raw & 64){ // IRQ Enable
            bus->setIF(3 + number, true); // Interrupt Flag für Timer 0 startet bei bit 3
        }
        cbus->timerOverflowed(number);
        if(!usesPreviousTimer()){
            reschedule();
        }
    }
}

template<typename ConcreteBus>
bool Timer<ConcreteBus>::usesPreviousTimer()
{
    return (control.raw & 4u) && number > 0; // Geht nur, wenn das nicht der erste Timer (t0) ist
}

template<typename ConcreteBus>
void Timer<ConcreteBus>::clockWithPrevious() {
    if(control.raw & 128){
        onIncrement();
    }
}

template<typename ConcreteBus>
void Timer<ConcreteBus>::onIncrement() {
    this->value++;
    if(this->value > 0xFFFF){ // Overflow
        overflowTimer();
    }
}

template class Timer<Bus7>;
template class Timer<Bus9>;
