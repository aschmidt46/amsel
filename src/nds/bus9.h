#pragma once
#include "arm/arm946e-s.h"
#include "gba/ibus.h"
#include "gba/register/general_purpose.h"

namespace nds{
    class SharedBus;

    class Bus9 : public gba::IBus<InstructionInfo>{
        SharedBus* sharedBus;
        CPU cpu;

        size_t clocks = 0;

        std::vector<Byte> bios9;

        Word IE = 0;
        Word IF = 0;
        gba::GeneralPurpose32 IME;

        public:
        Bus9(SharedBus* sb) : sharedBus(sb), IME(0x4000208){};
        void writeByte(Word addr, Byte val);
        Byte readByte(Word addr);
        void writeByteFromWide(Word addr, Byte val);
        Byte readByteFromWide(Word addr);
        void writeHalfWord(Word addr, HalfWord val);
        HalfWord readHalfWord(Word addr);
        void writeWord(Word addr, Word val);
        Word readWord(Word addr);

        void addCPUCycles(size_t num);

        void loadBios(const std::vector<Byte> &content);
        size_t getBiosSize();

        size_t getClocks();
        void timerOverflowed(int index);

        void setIF(int bit, bool val);
        
        Word getIE();
        Word getIF();
        bool hasIME();

        void setHalt();
    };
}
