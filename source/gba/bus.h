#pragma once

#include "dma.h"
#include "ibus.h"
#include "arm/arm7tdmi.h"
#include "ppu.h"
#include <vector>
#include "register/general_purpose.h"
#include "timer.h"

namespace gba{
    class Bus final : public IBus, virtual public std::enable_shared_from_this<Bus>{

        PPU ppu;
        CPU cpu;
        std::vector<Timer> timers; // 0,1,2,3
        std::vector<DMAChannel> dma; // 0,1,2,3
        std::vector<Byte> bios;

        // Register
        HalfWord IF = 0;
        HalfWord IE = 0;
        GeneralPurpose32 IME = 0;
        GeneralPurpose32 waitCNT = 0;
        GeneralPurpose16 KEYINPUT = 0;
        GeneralPurpose16 KEYCNT = 0;
        GeneralPurpose32 InternalMemoryControl = 0;

        Byte POSTFLG = 0;
        Byte HALTCNT = 0;


        std::vector<Byte> wramBoard;
        std::vector<Byte> wramChip;
        std::vector<Byte> gamePak;
        std::vector<Byte> cartRam;
        // std::vector<Byte> eeprom;
        // class EEPROM
        Byte null = 0;
        Bus();
        bool halted = false;
        int steps = 0;
        bool watchBreakpoints = false;
        std::vector<std::string> breakpointsOP;
        std::vector<uint64_t> breakpoints;


        public:
        unsigned int getCyclesForAccess(Word addr, bool sequential);
        void init();
        Bus(const char *path);
        Bus(const std::vector<Byte> &bytes);
        ~Bus() = default;
        // Adressen vorher noch alignen?
        void writeByte(Word addr, Byte val) override;
        void writeByteFromWide(Word addr, Byte val);
        Byte readByte(Word addr) override;
        void writeHalfWord(Word addr, HalfWord val) override;
        HalfWord readHalfWord(Word addr) override;
        void writeWord(Word addr, Word val) override;
        Word readWord(Word addr) override;

        void press(int i);
        void release(int i);

        void setIF(int bit, bool value);

        void clock();

        void PPUEnteredHBlank();
        void PPULeftHBlank();
        void PPUEnteredVBlank();
        void PPULeftVBlank();

        uint32_t* accessFramebuffer();
        bool hasFrame();
        void setHalt(bool to);
        bool isHalted();
        void addStep();
        std::pair<std::string, std::vector<int>> getNextInstructions();
        std::pair<std::string, std::vector<int>> getPrevInstructions();
        std::vector<std::string> removeBreakpointOP(std::string bp);
        std::vector<std::string> addBreakpointOP(std::string bp);
        CpuRegisterState getRegs();
        std::vector<std::string> getStack();
        std::string getMode();
        std::vector<uint64_t> addBreakpoint(uint64_t bp);
        std::vector<uint64_t> removeBreakpoint(uint64_t bp);
        void setHalt() override;
        std::string getDisassembly(uint64_t code);
        std::tuple<std::string, std::string, std::string> getLastTransaction();

        void loadBios(const std::vector<uint8_t> &content);
    };
}
