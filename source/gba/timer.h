#pragma once

#include "arm/bus_types.h"
#include <memory>



namespace gba{
    class Bus;
    static const unsigned int timerDividers[4] = {1, 64, 256, 1024};
    class Timer{
        private:
        Word value;
        Word dividerValue;
        int number;

        std::weak_ptr<Bus> bus;

        void onIncrement();
        bool overflow = false;

        public:
        Timer(int num, std::weak_ptr<Bus> bptr) : number(num), bus(bptr){};
        HalfWord reload;
        HalfWord control;

        // void onReloadWriteHW(HalfWord val);
        // void onControlWriteHW(HalfWord val);
        // void onReloadWriteBLower(Byte val);
        // void onReloadWriteBHigher(Byte val);
        // void onControlWriteBLower(Byte val);
        // void onControlWriteBHigher(Byte val);
        // void onWordWrite(Word val);

        bool usesPreviousTimer();

        void clock(); // Count erhöhen anhand Prescaler
        void clockWithPrevious(); // Count erhöhen durch anderen Timer
        bool justOverflowed();
    };
}
