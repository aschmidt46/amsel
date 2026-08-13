#pragma once

#include "arm/bus_types.h"
#include "register/general_purpose.h"
#include <memory>



namespace gba{
    class Bus;
    static const unsigned int timerDividers[4] = {1, 64, 256, 1024};
    class Timer{
        private:
        Word value;
        Word dividerValue;
        int number;

        Bus* bus;

        void onIncrement();
        bool overflow = false;

        public:
        Timer() : reload(0), control(0){};
        Timer(int num, Bus* bptr) : number(num), bus(bptr), reload(0), control(2){};
        GeneralPurpose16 reload;
        GeneralPurpose16 control;

        void onWrite(Word addr, Byte val);
        Byte onRead(Word addr);

        bool usesPreviousTimer();

        void clock(); // Count erhöhen anhand Prescaler
        void clockWithPrevious(); // Count erhöhen durch anderen Timer
        bool justOverflowed();
    };
}
