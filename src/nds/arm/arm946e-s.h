#pragma once
#include <fstream>
#include <optional>
#include <string>
#include <memory>
#include <vector>
#include "arm946e-s_types.h"
#include "cp15.h"


namespace gba{
    template<typename T>
    class IBus;
}

namespace nds{


    inline uint32_t sign_extend_n_32(uint32_t x, uint32_t bits) {
        uint32_t m = 1u << (bits - 1);
        return (x ^ m) - m;
    }

    class CPU{
        // Register
        Word _R0 = 0, _R1 = 0, _R2 = 0, _R3 = 0, _R4 = 0, _R5 = 0, _R6 = 0, _R7 = 0, _R8 = 0, _R9 = 0, _R10 = 0, _R11 = 0, _R12 = 0, _R13_SP = 0, _R14_L_R = 0, _R15_PC = 0xFFFF0000,
        _R8_fiq = 0, _R9_fiq = 0, _R10_fiq = 0, _R11_fiq = 0, _R12_fiq = 0, _R13_fiq = 0, _R14_fiq = 0, _R13_SVC = 0, _R14_SVC = 0, _R13_abt = 0, _R14_abt = 0, _R13_irq = 0, _R14_irq = 0, _R13_und = 0, _R14_und = 0;

        StatusRegister _CPSR, _SPSR_fiq, _SPSR_SVC, _SPSR_abt, _SPSR_irq, _SPSR_und; // Init in Konstruktor

        bool haltMode = false;

        public:
        inline OperatingMode mode() const{
            return (OperatingMode)_CPSR.state.mode_bits;
        }

        inline CpuState state() const{
            switch(_CPSR.state.T){
                case 0: return ARM;
                case 1: return THUMB;
                default: return ARM;
            }
        }

        private:
        inline Word pcInterval() const{
            return (this->state() == ARM ? 4 : 2);
        }
        inline bool systemUser() const{
            return mode() == System || mode() == User;
        }

        Word* const registerMap[32][18] = {
            {nullptr}, {nullptr}, {nullptr}, {nullptr}, {nullptr}, {nullptr}, {nullptr}, {nullptr}, {nullptr}, {nullptr}, {nullptr}, {nullptr}, {nullptr}, {nullptr}, {nullptr}, {nullptr},
            { // 16 User
                &_R0, &_R1, &_R2, &_R3, &_R4, &_R5, &_R6, &_R7,
                &_R8, &_R9, &_R10, &_R11, &_R12, &_R13_SP, &_R14_L_R, &_R15_PC, &_CPSR.raw, nullptr
            },
            { // 17 FIQ
                &_R0, &_R1, &_R2, &_R3, &_R4, &_R5, &_R6, &_R7,
                &_R8_fiq, &_R9_fiq, &_R10_fiq, &_R11_fiq, &_R12_fiq, &_R13_fiq, &_R14_fiq, &_R15_PC, &_CPSR.raw, &_SPSR_fiq.raw
            },{ // 18 IRQ
                &_R0, &_R1, &_R2, &_R3, &_R4, &_R5, &_R6, &_R7,
                &_R8, &_R9, &_R10, &_R11, &_R12, &_R13_irq, &_R14_irq, &_R15_PC, &_CPSR.raw, &_SPSR_irq.raw
            },{ // 19 SVC
                &_R0, &_R1, &_R2, &_R3, &_R4, &_R5, &_R6, &_R7,
                &_R8, &_R9, &_R10, &_R11, &_R12, &_R13_SVC, &_R14_SVC, &_R15_PC, &_CPSR.raw, &_SPSR_SVC.raw
            }, {nullptr}, {nullptr}, {nullptr}, { // 23 ABT
                &_R0, &_R1, &_R2, &_R3, &_R4, &_R5, &_R6, &_R7,
                &_R8, &_R9, &_R10, &_R11, &_R12, &_R13_abt, &_R14_abt, &_R15_PC, &_CPSR.raw, &_SPSR_abt.raw
            },{nullptr}, {nullptr}, {nullptr}, { // 27 UND
                &_R0, &_R1, &_R2, &_R3, &_R4, &_R5, &_R6, &_R7,
                &_R8, &_R9, &_R10, &_R11, &_R12, &_R13_und, &_R14_und, &_R15_PC, &_CPSR.raw, &_SPSR_und.raw
            },{nullptr}, {nullptr}, {nullptr}, { // 31 System
                &_R0, &_R1, &_R2, &_R3, &_R4, &_R5, &_R6, &_R7,
                &_R8, &_R9, &_R10, &_R11, &_R12, &_R13_SP, &_R14_L_R, &_R15_PC, &_CPSR.raw, nullptr
            }};


            
        Byte readByte(Word addr); // So machen, dass jedes readByte in ein readWord umgewandelt wird und dann zusammengesteckt wird? ist glaube ich korrekt
        // ACHTUNG LITTLE ENDIAN
        HalfWord readHalfWord(Word addr);
        HalfWord readHalfWordUnaligned(Word addr);
        Word readWord(Word addr);
        Word readWordUnaligned(Word addr);
        void writeByte(Word addr, Byte val);
        void writeHalfWord(Word addr, HalfWord val);
        void writeHalfWordUnaligned(Word addr, HalfWord val);
        void writeWord(Word addr, Word val);
        void writeWordUnaligned(Word addr, Word val);
        bool checkCondition(Condition c) const;
        
        
        
        // Pipeline Zustand
        InstructionInfo pipeline[2] = {};
        
        size_t remainingCycles = 0; // Extreme Vereinfachung, muss ich ändern


        gba::IBus<InstructionInfo>* bus;

        bool shouldFlush = false;
            
        public:

        CP15* cp15;

        bool lastTransactionWasRead = true;
        Word lastTransactionAddress = 0;
        Word lastTransactionData = 0;

        bool wasInterrupt = false;

        InstructionInfo decodeInstruction(Word code) const;
        static InstructionInfo decodeInstructionARM(Word code);
        static InstructionInfo decodeInstructionTHUMB(Word code);
        static std::string printInstructionType(InstructionType t);
        bool executeInstruction();
        
        // ARM
        // return -> Sprung ja nein
        bool executeBranchExchange(Word instruction);
        bool executeSingleDataSwap(Word instruction);
        bool executeBranchLink(Word instruction);
        bool executeBlockDataTransfer(Word instruction);
        bool executeSoftwareInterrupt(Word instruction);
        bool executeCoProcDataOperation(Word instruction);
        bool executeCoProcDataTransfer(Word instruction);
        bool executeCoProcRegTransfer(Word instruction);
        bool executeUndefined(Word instruction);
        bool executeSingleDataTransfer(Word instruction);
        bool executeDataTransferSignHDW(Word instruction);
        bool executeMultiply(Word instruction);
        bool executeMultiplyLong(Word instruction);
        bool executeDataProc(Word instruction);
        bool executePSRTransfer(Word instruction);

        bool executeCountLeadingZeros(Word instruction);
        bool executeQALU(Word instruction);
        bool executeMultiplyHalfWord(Word instruction);
        
        // THUMB
        
        bool executeThumbMoveShiftedRegister(Word instruction);
        bool executeThumbAddSubtract(Word instruction);
        bool executeThumbMoveCompareAddSubtractImmediate(Word instruction);
        bool executeThumbALUOperations(Word instruction);
        bool executeThumbHiRegisterOperationsBX(Word instruction);
        bool executeThumbPCRelativeLoad(Word instruction);
        bool executeThumbLoadStoreWithRegisterOff(Word instruction);
        bool executeThumbLoadStoreSignExtendedBHW(Word instruction);
        bool executeThumbLoadStoreWithImmOffset(Word instruction);
        
        bool executeThumbLoadStoreHalfWord(Word instruction);
        bool executeThumbSPRelativeLoadStore(Word instruction);
        bool executeThumbLoadAddress(Word instruction);
        bool executeThumbAddOffsetToSP(Word instruction);
        bool executeThumbPushPopRegisters(Word instruction);
        bool executeThumbMultipleLoadStore(Word instruction);
        bool executeThumbConditionalBranch(Word instruction);
        bool executeThumbSoftwareInterrupt(Word instruction);
        bool executeThumbUnconditionalBranch(Word instruction);
        bool executeThumbLongBranchWithLink(Word instruction);
        
        bool executeThumbBranchExchange(Word instruction);
        
        void executeHardwareInterrupt();

        // Debug
        bool advanced = false;
        
        
        public:
        CPU(bool skipBios = true);
        std::ofstream outs;
        CPU(gba::IBus<InstructionInfo>* bus, CP15* cp15) : CPU(false){
            this->bus = bus;
            this->cp15 = cp15;
            this->outs = std::ofstream("arm9log.txt");
        };
        bool startLogging = false;

        void initHLEBoot();
        
        // führt die nächste Instruktion aus, falls das möglich ist
        void advanceCPU();
        // Führt eine Instruktion aus und clockt die CPU so oft, bis sie erneut eine Instruktion ausführen kann
        void advanceCPUToNextValidState();

        void enterHaltMode();

        Word getPC() const{
            return _R15_PC;
        };
        void setPC(Word val){
            _R15_PC = val;
        }
        void setLR(Word val){
            _R14_L_R = val;
        }
        void setIP(Word val){
            _R12 = val;
        }

        void clock();
        bool pollInterrupts();

        bool pipelineIsSaturated();
        void flushPipeline();
        template<bool isARM, bool testMode>
        void advancePipeline();
        void addCycles(size_t cycles);
        size_t armCacheSize = 0;
        size_t thumbCacheSize = 0;
        size_t armCacheSizeBIOS = 0;
        size_t thumbCacheSizeBIOS = 0;


        // Debug
        bool debug = false;
        bool pipelineHasValue();
        bool advancedThisClock();
        std::pair<std::string, std::vector<int>> getNextNInstructions(int n);
        std::pair<std::string, std::vector<int>> getPrev10Instructions();
        void incrementCircular();
        const int circSize = 20;
        std::vector<std::pair<int64_t, CpuState>> circular{std::vector<std::pair<int64_t, CpuState>>(circSize,{-1, ARM})};
        unsigned int circularIndex = 0;
        std::string getCurrentOpcode();


        // Tests
        CpuRegisterState getRegisterState() const;
        void setRegisterState(CpuRegisterState state);
        InstructionInfo getInstructionInPipeline();
        std::string printCPSR();
        std::string printMode();
        std::string getDisassembly(Word code, Word pc = 0);

    };
};
