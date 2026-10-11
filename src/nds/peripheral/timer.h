#pragma once

#include "../bus_types.h"
#include "gba/register/general_purpose.h"
#include <memory>
#include "../scheduler.h"



namespace nds{
    class SharedBus;
    static const unsigned int timerDividers[4] = {1, 64, 256, 1024};

    template<typename ConcreteBus>
    class Timer{
        private:
        const EventType scheduleEvent = NDSEVENT_TimerOverflow7;
        size_t numStart = 0;
        Word value;
        int number;
        size_t clocksStart = 0;

        SharedBus* bus;
        ConcreteBus* cbus;

        void onIncrement();

        public:
        Timer() : reload(0), control(0){};
        Timer(int num, SharedBus* bptr, ConcreteBus* cbus, EventType e) : scheduleEvent(e), number(num), bus(bptr), cbus(cbus), reload(0), control(2){};
        gba::GeneralPurpose16 reload;
        gba::GeneralPurpose16 control;

        void onWrite(Word addr, Byte val);
        Byte onRead(Word addr);

        void reschedule();
        void overflowTimer();

        bool usesPreviousTimer();

        void clockWithPrevious(); // Count erhöhen durch anderen Timer
    };
}
