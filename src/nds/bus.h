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
        bool halted = false;
        bool watchBreakpoints = false;
        std::vector<uint64_t> breakpoints;
        std::vector<std::string> breakpointsOP;
        public:
        bool watchARM7 = false;
        int steps = 0;
        std::shared_ptr<Bus7> bus7;
        std::shared_ptr<Bus9> bus9;
        std::shared_ptr<IPC> ipc;
        std::shared_ptr<LCD> lcd;
        std::shared_ptr<gba::Scheduler<EventType, SharedBus>> scheduler;
        
        
        std::array<Byte, 0x40'0000> mainMemory;
        std::array<Byte, 0x8000> sharedWRAM = {};
        Byte wramCNT = 0;
        std::vector<Byte> firmware;
        
        std::vector<Byte> gameCart;
        gba::GeneralPurpose16 KEYINPUT;
        gba::GeneralPurpose16 KEYCNT;
        gba::GeneralPurpose16 EXKEYINPUT;
        HalfWord EXMEMCNT = 0b11 << 13; // 7-15 shared
        
        SharedBus() : KEYINPUT(0x04000130), KEYCNT(0x04000132), EXKEYINPUT(0x04000136){
            KEYINPUT.raw = 0b1111111111;
            EXKEYINPUT.raw = 0b01111111; // Hinge (bit 7) offen = 0
        };
        void loadCartHLE(const std::vector<uint8_t> &bytes);
        Word nds7romRamStart = 0;
        Word nds7romRomStart = 0;
        Word nds7romSize = 0;
        
        Word nds9romRamStart = 0;
        Word nds9romRomStart = 0;
        Word nds9romSize = 0;
        
        
        Byte readByte(Word addr);
        void writeByte(Word addr, Byte val);

        bool canAccessGameCart(const bool isNDS9) const;

        // modulo, offset
        std::array<std::pair<Word, Word>, 4> wramAllocationNDS7 = { { {0, 0}, {0x4000, 0}, {0x4000, 0x4000}, {0x8000, 0} } };
        std::array<std::pair<Word, Word>, 4> wramAllocationNDS9 = { { {0x8000, 0}, {0x4000, 0x4000}, {0x4000, 0}, {0, 0} } };


        void PPUEnteredHBlank();
        void PPUEnteredVBlank();
        void PPULeftHBlank();
        void PPULeftVBlank();

        void init();
        void setIF(int bit, bool val);
        void setIFSingleBus(int bit, bool val, bool nds9);

        void setHalt();

        uint32_t* accessFramebuffer();


        // Interface

        std::pair<float, float> getSample();
        void clock();
        void press(int i);
        void release(int i);



        // Debug



        void setDebug(bool to);
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
        std::string getState();
        std::vector<uint64_t> addBreakpoint(uint64_t bp);
        std::vector<uint64_t> removeBreakpoint(uint64_t bp);
        std::string getDisassembly(uint64_t code);
        std::tuple<std::string, std::string, std::string> getLastTransaction();

        void extendedDebugMenu();
        void memdump9(std::ofstream &out);
    };

}
