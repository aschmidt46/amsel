#pragma once
#include "arm/arm946e-s.h"
#include "gba/ibus.h"
#include "gba/register/general_purpose.h"
#include "peripheral/maths.h"
#include "peripheral/timer.h"
#include "peripheral/dma.h"

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
        bool watchBreakpoints = false;
        std::vector<uint64_t> breakpoints;
        std::vector<std::string> breakpointsOP;
        DSMaths maths;
        std::array<Timer<Bus9>, 4> timers;
        std::array<DMAChannel<Bus9>, 4> dma;

        std::array<gba::GeneralPurpose32, 4> dmaFill = {{gba::GeneralPurpose32(0x40000E0), gba::GeneralPurpose32(0x40000E4), gba::GeneralPurpose32(0x40000E8), gba::GeneralPurpose32(0x40000EC)}};

        Byte exMem06 = 0;
        Byte POSTFLG = 0;
        HalfWord POWCNT1 = 0;
        
        CPU cpu;
        CP15 cp15;
        Bus9(SharedBus* sb) : sharedBus(sb), itcm(0x8000, 0), dtcm(0x4000, 0), IME(0x4000208){};
        void init();
        void writeByte(Word addr, Byte val);
        Byte readByte(Word addr);
        inline void writeByteFromWide(Word addr, Byte val){
            writeByteFromWide(addr, val, false);
        };
        inline Byte readByteFromWide(Word addr){
            return readByteFromWide(addr, false);
        };
        inline void writeHalfWord(Word addr, HalfWord val){
            writeHalfWord(addr, val, false);
        };
        inline HalfWord readHalfWord(Word addr){
            return readHalfWord(addr, false);
        };
        inline void writeWord(Word addr, Word val){
            writeWord(addr, val, false);
        };
        inline Word readWord(Word addr){
            return readWord(addr, false);
        };

        inline void writeHalfWordFromDMA(Word addr, HalfWord val){
            writeHalfWord(addr, val, true);
        };
        inline HalfWord readHalfWordFromDMA(Word addr){
            return readHalfWord(addr, true);
        };
        inline void writeWordFromDMA(Word addr, Word val){
            writeWord(addr, val, true);
        };
        inline Word readWordFromDMA(Word addr){
            return readWord(addr, true);
        };

        void writeByteFromWide(Word addr, Byte val, bool isDMA);
        Byte readByteFromWide(Word addr, bool isDMA);

        void writeHalfWord(Word addr, HalfWord val, bool isDMA);
        HalfWord readHalfWord(Word addr, bool isDMA);
        void writeWord(Word addr, Word val, bool isDMA);
        Word readWord(Word addr, bool isDMA);

        void PPUEnteredHBlank();
        void PPUEnteredVBlank();
        void onDataWordArrived();

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

        void onWritePOWCNT1();


        std::vector<uint64_t> addBreakpoint(uint64_t bp);
        std::vector<uint64_t> removeBreakpoint(uint64_t bp);
        std::vector<std::string> removeBreakpointOP(std::string bp);
        std::vector<std::string> addBreakpointOP(std::string bp);
    };
}
