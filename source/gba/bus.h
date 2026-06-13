#pragma once

#include "ibus.h"
#include "arm/arm7tdmi.h"
#include "ppu.h"
#include <vector>
#include "timer.h"

namespace gba{
    class Bus : public IBus, virtual public std::enable_shared_from_this<Bus>{

        PPU ppu;
        CPU cpu;
        std::vector<Timer> timers; // 0,1,2,3
        std::vector<Byte> bios;

        // Register
        HalfWord IF = 0;
        HalfWord IE = 0;
        Word IME = 0;

        std::vector<Byte> wramBoard;
        std::vector<Byte> wramChip;
        std::vector<Byte> gamePak;
        Byte null = 0;
        Byte* accessMemory(Word addr);
        Bus();
        bool halted = false;
        int steps = 0;
        public:
        void init();
        Bus(const char *path);
        Bus(const std::vector<Byte> &bytes);
        // Adressen vorher noch alignen?
        void writeByte(Word addr, Byte val) override;
        Byte readByte(Word addr) override;
        void writeHalfWord(Word addr, HalfWord val) override;
        HalfWord readHalfWord(Word addr) override;
        void writeWord(Word addr, Word val) override;
        Word readWord(Word addr) override;

        void setIF(int bit, bool value);

        void clock();

        float* accessFramebuffer();
        bool hasFrame();
        void setHalt(bool to);
        bool isHalted();
        void addStep();
        std::pair<std::string, std::vector<int>> getNextInstructions();
        std::pair<std::string, std::vector<int>> getPrevInstructions();
    };
}
