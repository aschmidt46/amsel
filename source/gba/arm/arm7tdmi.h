#pragma once
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

        inline OperatingMode mode() const{
            switch(_CPSR.state.mode_bits){
                case 0: return SystemUser;      // ;\26bit Backward Compatibility modes
                case 1: return FIQ;             // ; (supported only on ARMv3, except ARMv3G,
                case 2: return IRQ;             // ; and on some non-T variants of ARMv4)
                case 3: return Supervisor;      // ;/
                case 16: return SystemUser;
                case 17: return FIQ;
                case 18: return IRQ;
                case 19: return Supervisor;
                case 23: return AbortMode;
                case 27: return Undefined;
                case 31: return SystemUser;
                default: return SystemUser;
            }
        }

        inline CpuState state() const{
            switch(_CPSR.state.T){
                case 0: return ARM;
                case 1: return THUMB;
                default: return ARM;
            }
        }
        inline Word pcInterval() const{
            return (this->state() == ARM ? 4 : 2);
        }

        Word* const registerMap[OP_MODES][18] = {{
                &_R0, &_R1, &_R2, &_R3, &_R4, &_R5, &_R6, &_R7,
                &_R8, &_R9, &_R10, &_R11, &_R12, &_R13_SP, &_R14_L_R, &_R15_PC, &_CPSR.raw, nullptr
            },{
                &_R0, &_R1, &_R2, &_R3, &_R4, &_R5, &_R6, &_R7,
                &_R8_fiq, &_R9_fiq, &_R10_fiq, &_R11_fiq, &_R12_fiq, &_R13_fiq, &_R14_fiq, &_R15_PC, &_CPSR.raw, &_SPSR_fiq.raw
            },{
                &_R0, &_R1, &_R2, &_R3, &_R4, &_R5, &_R6, &_R7,
                &_R8, &_R9, &_R10, &_R11, &_R12, &_R13_SVC, &_R14_SVC, &_R15_PC, &_CPSR.raw, &_SPSR_SVC.raw
            },{
                &_R0, &_R1, &_R2, &_R3, &_R4, &_R5, &_R6, &_R7,
                &_R8, &_R9, &_R10, &_R11, &_R12, &_R13_abt, &_R14_abt, &_R15_PC, &_CPSR.raw, &_SPSR_abt.raw
            },{
                &_R0, &_R1, &_R2, &_R3, &_R4, &_R5, &_R6, &_R7,
                &_R8, &_R9, &_R10, &_R11, &_R12, &_R13_irq, &_R14_irq, &_R15_PC, &_CPSR.raw, &_SPSR_irq.raw
            },{
                &_R0, &_R1, &_R2, &_R3, &_R4, &_R5, &_R6, &_R7,
                &_R8, &_R9, &_R10, &_R11, &_R12, &_R13_und, &_R14_und, &_R15_PC, &_CPSR.raw, &_SPSR_und.raw
            }};


            
        Byte readByte(Word addr); // So machen, dass jedes readByte in ein readWord umgewandelt wird und dann zusammengesteckt wird? ist glaube ich korrekt
        // ACHTUNG LITTLE ENDIAN
        HalfWord readHalfWord(Word addr);
        HalfWord readHalfWordUnaligned(Word addr);
        Word readWord(Word addr);
        Word readWordUnaligned(Word addr);
        void writeByte(Word addr, Byte val);
        void writeHalfWord(Word addr, HalfWord val);
        void writeWord(Word addr, Word val);
        void writeWordUnaligned(Word addr, Word val);
        bool checkCondition(Condition c) const;
        
        
        
        // Pipeline Zustand
        std::optional<Word> pipelineRead;
        std::optional<InstructionInfo> pipelineDecoded;
        
        size_t remainingCycles = 0; // Extreme Vereinfachung, muss ich ändern


        std::weak_ptr<gba::IBus> bus;

        bool shouldFlush = false;
            
        public:
        std::vector<Byte> boardWRAM = std::vector<Byte>(0x40000, 0);// 02000000-0203FFFF   WRAM - On-board Work RAM  (256 KBytes) 2 Wait

        InstructionInfo decodeInstruction(Word code);
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
        
        private: CPU();
        public:
        CPU(std::shared_ptr<IBus> bus) : CPU(){
            this->bus = bus;
        };
        
        // führt die nächste Instruktion aus, falls das möglich ist
        void advanceCPU();
        // Führt eine Instruktion aus und clockt die CPU so oft, bis sie erneut eine Instruktion ausführen kann
        void advanceCPUToNextValidState();
        bool pipelineIsSaturated();
        void flushPipeline();
        void advancePipeline();


        // Tests
        CpuRegisterState getRegisterState() const;
        void setRegisterState(CpuRegisterState state);
        InstructionInfo getInstructionInPipeline();
        std::string printCPSR();
        std::string printMode();

    };
};
