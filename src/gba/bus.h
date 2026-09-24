#pragma once

#include "dma.h"
#include "ibus.h"
#include "arm/arm7tdmi.h"
#include "ppu/ppu.h"
#include "apu.h"
#include <vector>
#include "register/general_purpose.h"
#include "timer.h"
#include <array>
#include <cstring>
#include "peripheral/flash.h"

enum BackupType{
    BACKUP_EEPROM,
    BACKUP_SRAM,
    BACKUP_FLASH_64,
    BACKUP_FLASH_128,
    BACKUP_NO_BACKUP
};

namespace gba{
    class Bus final : public IBus, virtual public std::enable_shared_from_this<Bus>{

        PPU ppu;
        CPU cpu;
        APU apu;
        std::array<Timer, 4> timers; // 0,1,2,3
        public:
        std::array<DMAChannel, 4> dma; // 0,1,2,3
        private:
        std::vector<Byte> bios;
        size_t clocks = 0;

        // Nicht immer vorhanden
        std::unique_ptr<Flash> flash = nullptr;

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


        std::array<Byte, 0x40000>* wramBoard;
        std::array<Byte, 0x8000>* wramChip;
        std::vector<Byte> gamePak;
        std::array<Byte, 0x10000>* cartRam;
        // std::vector<Byte> eeprom;
        // class EEPROM
        Byte null = 0;
        Bus();
        bool halted = false;
        int steps = 0;
        bool watchBreakpoints = false;
        std::vector<std::string> breakpointsOP;
        std::vector<uint64_t> breakpoints;

        BackupType backupType = BACKUP_NO_BACKUP;

        
        
        public:
        void timerOverflowed(int number);
        std::pair<float, float> getSample();
        HalfWord getIE() override;
        HalfWord getIF() override;
        bool hasIME() override;
        unsigned int getCyclesForAccess(Word addr, bool sequential);
        void init(bool skipBios);
        Bus(const char *path, const char* biosPath);
        Bus(const std::vector<Byte> &bytes);
        ~Bus(){
            delete wramBoard;
            delete wramChip;
            delete cartRam;
        };
        void determineBackup();
        bool canSave();
        std::vector<uint8_t> getSaveData();
        size_t getSaveSize();
        void loadSave(const std::vector<uint8_t> &saveData);
        // Adressen vorher noch alignen?
        void writeByte(Word addr, Byte val) override;
        void writeByteFromWide(Word addr, Byte val);
        Byte readByteFromWide(Word addr);
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
        void setHalt() override;
        std::string getDisassembly(uint64_t code);
        std::tuple<std::string, std::string, std::string> getLastTransaction();

        void loadBios(const std::vector<uint8_t> &content);
    };
}
