#pragma once
#include "bus7.h"
#include "bus9.h"
#include "gba/register/general_purpose.h"
#include "gba/scheduler.h"
#include "lcd/lcd.h"
#include "scheduler.h"

namespace nds{

    class SharedBus{
        bool isHalted = false;
        Word IE = 0;
        Word IF = 0;
        gba::GeneralPurpose32 IME;
        public:
        SharedBus() : IME(0x4000208){};
        std::shared_ptr<Bus7> bus7;
        std::shared_ptr<Bus9> bus9;
        std::shared_ptr<LCD> lcd;
        std::shared_ptr<gba::Scheduler<EventType, SharedBus>> scheduler;

        void PPUEnteredHBlank();
        void PPUEnteredVBlank();
        void PPULeftHBlank();
        void PPULeftVBlank();

        void init();
        void setIF(int bit, bool val);

        Word getIE();
        Word getIF();
        bool hasIME();

        void setHalt();


        // Interface

        std::pair<float, float> getSample();
        void clock();
    };

}
