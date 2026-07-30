#pragma once
#include <fstream>
#include <optional>
#include <string>
#include <memory>
#include "../ibus.h"
#include <vector>
#include "arm7tdmi_types.h"

namespace gba{

    class IBus;

    inline uint32_t sign_extend_n_32(uint32_t x, uint32_t bits) {
        uint32_t m = 1u << (bits - 1);
        return (x ^ m) - m;
    }

    class CPU{
        // Register
        Word _R0 = 0, _R1 = 0, _R2 = 0, _R3 = 0, _R4 = 0, _R5 = 0, _R6 = 0, _R7 = 0, _R8 = 0, _R9 = 0, _R10 = 0, _R11 = 0, _R12 = 0, _R13_SP = 0, _R14_L_R = 0, _R15_PC = 0,
        _R8_fiq = 0, _R9_fiq = 0, _R10_fiq = 0, _R11_fiq = 0, _R12_fiq = 0, _R13_fiq = 0, _R14_fiq = 0, _R13_SVC = 0, _R14_SVC = 0, _R13_abt = 0, _R14_abt = 0, _R13_irq = 0, _R14_irq = 0, _R13_und = 0, _R14_und = 0;

        StatusRegister _CPSR, _SPSR_fiq, _SPSR_SVC, _SPSR_abt, _SPSR_irq, _SPSR_und; // Init in Konstruktor

        public:
        inline OperatingMode mode() const{
            return (OperatingMode)_CPSR.state.mode_bits;
            // switch(_CPSR.state.mode_bits){
                // case 0: return User;      // ;\26bit Backward Compatibility modes
                // case 1: return FIQ;             // ; (supported only on ARMv3, except ARMv3G,
                // case 2: return IRQ;             // ; and on some non-T variants of ARMv4)
                // case 3: return Supervisor;      // ;/
                // case 16: return User;
                // case 17: return FIQ;
                // case 18: return IRQ;
                // case 19: return Supervisor;
                // case 23: return AbortMode;
                // case 27: return Undefined;
                // case 31: return System;
                // default:{
                //     // auto m = _CPSR.state.mode_bits;
                //     // if(m < 16){
                //     //     m &= 0b11;
                //     //     if(m==0) return User;
                //     //     if(m==1) return FIQ;
                //     //     if(m==2) return IRQ;
                //     //     if(m==3) return Supervisor;
                //     // }
                //     return User;
                // }
            // }
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
        int64_t pipelineRead = -1;
        InstructionInfo pipelineDecoded = {PipelineEmpty, 0};
        
        size_t remainingCycles = 0; // Extreme Vereinfachung, muss ich ändern


        IBus* bus;

        bool shouldFlush = false;
            
        public:

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
        void executeHardwareInterrupt();

        // Debug
        bool advanced = false;
        
        
        public:
        CPU();
        CPU(IBus* bus) : CPU(){
            this->bus = bus;
        };
        
        // führt die nächste Instruktion aus, falls das möglich ist
        void advanceCPU();
        // Führt eine Instruktion aus und clockt die CPU so oft, bis sie erneut eine Instruktion ausführen kann
        void advanceCPUToNextValidState();

        void clock();
        bool pollInterrupts();

        bool pipelineIsSaturated();
        void flushPipeline();
        void advancePipeline();


        // Debug
        bool pipelineHasValue();
        bool advancedThisClock();
        std::pair<std::string, std::vector<int>> getNextNInstructions(int n);
        std::pair<std::string, std::vector<int>> getPrev10Instructions();
        void incrementCircular();
        const int circSize = 15;
        std::vector<std::pair<int64_t, CpuState>> circular{std::vector<std::pair<int64_t, CpuState>>(circSize,{-1, ARM})};
        unsigned int circularIndex = 0;
        std::string getCurrentOpcode();


        // Tests
        CpuRegisterState getRegisterState() const;
        void setRegisterState(CpuRegisterState state);
        InstructionInfo getInstructionInPipeline();
        std::string printCPSR();
        std::string printMode();
        std::string getDisassembly(Word code);

    };
};
