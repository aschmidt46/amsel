#include "timer.h"
#include "bus.h"

// void gba::Timer::onReloadWriteHW(HalfWord val) { reload = val; }

// void gba::Timer::onControlWriteHW(HalfWord val)
// {
//     control
// }

// void gba::Timer::onReloadWriteBLower(Byte val) {
//     reload = (reload & ~0xFFFF) | val;
// }

// void gba::Timer::onReloadWriteBHigher(Byte val) {
//     reload = (reload & 0xFFFF) | (HalfWord(val) << 8);  
// }

// void gba::Timer::onControlWriteBLower(Byte val) {}

// void gba::Timer::onControlWriteBHigher(Byte val) {}

// void gba::Timer::onWordWrite(Word val){
//     onReloadWriteHW(val);
//     onControlWriteHW(val >> 16);
// }

bool gba::Timer::usesPreviousTimer()
{
    return (control & 4u) && number > 0; // Geht nur, wenn das nicht der erste Timer (t0) ist
}

// 16.78 MHz
void gba::Timer::clock() {
    overflow = false;
    if(control & 128){ // Start bit
        Word divider = control & 0b11;
        this->dividerValue++;
        if(dividerValue >= timerDividers[divider]){
            this->dividerValue = 0;
            onIncrement();
        }
    }
}

void gba::Timer::clockWithPrevious() {
    overflow = false;
    if(control & 128){
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
    if(this->value >= 0xFFFF){ // Overflow
        overflow = true;
        this->value = reload;
        if(control & 64){ // IRQ Enable
            bus.lock()->setIF(3+number, true); // Interrupt Flag für Timer 0 startet bei bit 3
        }
    }
}
