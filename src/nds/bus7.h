#pragma once
#include "gba/ibus.h"
#include "gba/arm/arm7tdmi.h"
#include "bus_types.h"
#include "arm/arm946e-s_types.h"
#include "gba/register/general_purpose.h"
#include "peripheral/timer.h"
#include "peripheral/dma.h"
#include "peripheral/spi.h"
#include "peripheral/rtc.h"


namespace nds{
    class SharedBus;

    class Bus7 : public gba::IBus<gba::InstructionInfo>{
        
        size_t clocks = 0;
        
        public:
        SharedBus* sharedBus;
        std::vector<Byte> bios7;
        private:
        std::vector<Byte> arm7WRAM;
        
        Word IE = 0;
        Word IF = 0;
        gba::GeneralPurpose32 IME;
        
        HalfWord SOUNDCNTSTUB = 0;
        HalfWord SOUNDBIASSTUB = 0;
        HalfWord RCNT = 0;
        std::array<Byte, 0x100> audioStub = {};

        std::array<Byte, 0x8000> wifi = {};
                
        public:
        bool watchBreakpoints = false;
        std::vector<uint64_t> breakpoints;
        std::vector<std::string> breakpointsOP;
        std::array<Timer<Bus7>, 4> timers;
        std::array<DMAChannel<Bus7>, 4> dma;
        Byte exMem06 = 0;
        Byte POSTFLG = 0;
        Byte POWCNT2 = 1;
        Byte WIFIWAITCNT = 0x30; // von Firmware gesetzt!
        gba::GeneralPurpose32 BIOSPROT;
        Byte HALTCNT = 0;
        gba::CPU cpu;
        SPI spi;
        std::unique_ptr<RTC> rtc = nullptr;
        Bus7(SharedBus* sb) : sharedBus(sb), arm7WRAM(0x10000, 0), IME(0x4000208), BIOSPROT(0x4000308){};
        void init();
        void writeByte(Word addr, Byte val);
        Byte readByte(Word addr);
        void writeByteFromWide(Word addr, Byte val);
        Byte readByteFromWide(Word addr);
        void writeHalfWord(Word addr, HalfWord val);
        HalfWord readHalfWord(Word addr);
        void writeWord(Word addr, Word val);
        Word readWord(Word addr);

        inline void writeHalfWordFromDMA(Word addr, HalfWord val){
            writeHalfWord(addr, val);
        };
        inline HalfWord readHalfWordFromDMA(Word addr){
            return readHalfWord(addr);
        };
        inline void writeWordFromDMA(Word addr, Word val){
            writeWord(addr, val);
        };
        inline Word readWordFromDMA(Word addr){
            return readWord(addr);
        };

        void PPUEnteredHBlank();
        void PPUEnteredVBlank();
        void onDataWordArrived();

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

        std::vector<uint64_t> addBreakpoint(uint64_t bp);
        std::vector<uint64_t> removeBreakpoint(uint64_t bp);
        std::vector<std::string> removeBreakpointOP(std::string bp);
        std::vector<std::string> addBreakpointOP(std::string bp);
    };
}
