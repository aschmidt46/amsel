#pragma once
#include "bus7.h"
#include "bus9.h"
#include "gba/register/general_purpose.h"
#include "gba/scheduler.h"
#include "lcd/lcd.h"
#include "scheduler.h"
#include "peripheral/ipc.h"

namespace nds{

    class SharedBus{
        bool isHalted = false;
        public:
        SharedBus(){};
        std::shared_ptr<Bus7> bus7;
        std::shared_ptr<Bus9> bus9;
        std::shared_ptr<IPC> ipc;
        std::shared_ptr<LCD> lcd;
        std::shared_ptr<gba::Scheduler<EventType, SharedBus>> scheduler;


        std::array<Byte, 0x40'0000> mainMemory = {};
        std::array<Byte, 0x8000> sharedWRAM = {};
        std::vector<Byte> firmware;


        void PPUEnteredHBlank();
        void PPUEnteredVBlank();
        void PPULeftHBlank();
        void PPULeftVBlank();

        void init();
        void setIF(int bit, bool val);
        void setIFSingleBus(int bit, bool val, bool nds9);

        void setHalt();


        // Interface

        std::pair<float, float> getSample();
        void clock();
    };

}
