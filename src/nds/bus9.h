#pragma once
#include "../gba/ibus.h"
#include "arm/arm946e-s.h"

namespace nds{
    class SharedBus;

    class Bus9 : public gba::IBus{
        SharedBus* sharedBus;
        CPU cpu;

        public:
        Bus9(SharedBus* sb) : sharedBus(sb){};
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
