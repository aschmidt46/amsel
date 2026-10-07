#pragma once
#include "arm/arm946e-s.h"
#include "gba/ibus.h"
#include "gba/register/general_purpose.h"

namespace nds{
    class SharedBus;

    class Bus9 : public gba::IBus<InstructionInfo>{
        SharedBus* sharedBus;
        
        size_t clocks = 0;
        
        std::vector<Byte> bios9;
        std::vector<Byte> itcm;
        std::vector<Byte> dtcm;
        
        Word IE = 0;
        Word IF = 0;
        gba::GeneralPurpose32 IME;
        public:
        gba::GeneralPurpose16 KEYINPUT;
        gba::GeneralPurpose16 KEYCNT;
        
        CPU cpu;
        CP15 cp15;
        Bus9(SharedBus* sb) : sharedBus(sb), itcm(0x8000, 0), dtcm(0x4000, 0), IME(0x4000208), KEYINPUT(0x04000130), KEYCNT(0x04000132){};
        void init();
        void writeByte(Word addr, Byte val);
        Byte readByte(Word addr);
        void writeByteFromWide(Word addr, Byte val);
        Byte readByteFromWide(Word addr);
        void writeHalfWord(Word addr, HalfWord val);
        HalfWord readHalfWord(Word addr);
        void writeWord(Word addr, Word val);
        Word readWord(Word addr);

        const std::vector<Byte> &getITCM() const{
            return itcm;
        };
        const std::vector<Byte> &getDTCM() const{
            return dtcm;
        };

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
