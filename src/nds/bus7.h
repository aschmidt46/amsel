#pragma once
#include "gba/ibus.h"
#include "gba/arm/arm7tdmi.h"
#include "bus_types.h"
#include "arm/arm946e-s_types.h"
#include "gba/register/general_purpose.h"

namespace nds{
    class SharedBus;

    class Bus7 : public gba::IBus<gba::InstructionInfo>{
        SharedBus* sharedBus;
        
        size_t clocks = 0;
        
        std::vector<Byte> bios7;
        std::vector<Byte> arm7WRAM;
        
        Word IE = 0;
        Word IF = 0;
        gba::GeneralPurpose32 IME;
        
        public:
        gba::CPU cpu;
        Bus7(SharedBus* sb) : sharedBus(sb), arm7WRAM(0x10000, 0), IME(0x4000208){};
        void init();
        void writeByte(Word addr, Byte val);
        Byte readByte(Word addr);
        void writeByteFromWide(Word addr, Byte val);
        Byte readByteFromWide(Word addr);
        void writeHalfWord(Word addr, HalfWord val);
        HalfWord readHalfWord(Word addr);
        void writeWord(Word addr, Word val);
        Word readWord(Word addr);

        void clock();

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
