#pragma once
#include "../gba/ibus.h"
#include "../gba/arm/arm7tdmi.h"
#include "bus_types.h"

namespace nds{
    class SharedBus;

    class Bus7 : public gba::IBus{
        SharedBus* sharedBus;
        gba::CPU cpu;

        public:
        Bus7(SharedBus* sb) : sharedBus(sb){};
        void writeByte(Word addr, Byte val);
        Byte readByte(Word addr);
        void writeHalfWord(Word addr, HalfWord val);
        HalfWord readHalfWord(Word addr);
        void writeWord(Word addr, Word val);
        Word readWord(Word addr);

        Word getIE();
        Word getIF();
        bool hasIME();

        void setHalt();
    };
}
