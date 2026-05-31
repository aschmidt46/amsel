#pragma once
#include "bus_types.h"
#include <optional>
#include <string>
#include <memory>
#include "../ibus.h"
#include <vector>

namespace gba{

    class IBus;

    struct CpuRegisterState{
        public:
        uint32_t R[16];
        uint32_t R_fiq[7];
        uint32_t R_svc[2];
        uint32_t R_abt[2];
        uint32_t R_irq[2];
        uint32_t R_und[2];
        uint32_t CPSR;
        uint32_t SPSR[5];
        uint32_t Pipeline[2];

        // Carry Flag ignorieren wegen Multiply Ungereimtheiten (https://bmchtech.github.io/post/multiply/)
        bool operator==(const CpuRegisterState& b) const{
            bool result = true;

            for(size_t i = 0; i < 16; i++){
                result &= this->R[i] == b.R[i];
            }
            for(size_t i = 0; i < 7; i++){
                result &= this->R_fiq[i] == b.R_fiq[i];
            }
            for(size_t i = 0; i < 2; i++){
                result &= this->R_svc[i] == b.R_svc[i];
            }
            for(size_t i = 0; i < 2; i++){
                result &= this->R_abt[i] == b.R_abt[i];
            }
            for(size_t i = 0; i < 2; i++){
                result &= this->R_irq[i] == b.R_irq[i];
            }
            for(size_t i = 0; i < 2; i++){
                result &= this->R_und[i] == b.R_und[i];
            }

            // Carry Flag und V ignorieren, V auch komisch bei MULL und MLAL
            result &= (this->CPSR & ~(0b11u << 28)) == (b.CPSR & ~(0b11u << 28));

            for(size_t i = 0; i < 5; i++){
                result &= this->SPSR[i] == b.SPSR[i];
            }
            for(size_t i = 0; i < 2; i++){
                result &= this->Pipeline[i] == b.Pipeline[i];
            }

            return result;
        };
    };

    struct Transaction{
        public:
        uint32_t kind;
        uint32_t size;
        uint32_t addr;
        uint32_t data;
        size_t cycle;
        uint32_t access;
    };

    enum CpuState{
        ARM = 0,
        THUMB = 1,
    };
    enum OperatingMode{
        SystemUser = 0,
        FIQ = 1,
        Supervisor = 2,
        AbortMode = 3,
        IRQ = 4,
        Undefined = 5,
    };
    static constexpr const int OP_MODES = 6;

    enum Condition{
        EQ = 0, NE = 1, CS = 2, CC = 3, MI = 4, PL = 5, VS = 6, VC = 7, HI = 8, LS = 9, GE = 10, LT = 11, GT = 12, LE = 13, AL = 14, NV = 15,
    };
    union StatusRegister {
        struct{
            Word mode_bits : 5; //lsb
            Word T : 1;
            Word F : 1;
            Word I : 1;
            Word A : 1;
            Word E : 1;
            Word Reserved : 17; // richtig?
            Word Q : 1;
            Word V : 1;
            Word C : 1;
            Word Z : 1;
            Word N : 1; // msb
        } state;
        Word raw;
    };

    enum RegisterEnum {R0 = 0, R1 = 1, R2 = 2, R3 = 3, R4 = 4, R5 = 5, R6 = 6, R7 = 7, R8 = 8, R9 = 9, R10 = 10, R11 = 11, R12 = 12, R13 = 13, R14 = 14, R15 = 15, CPSR = 16, SPSR = 17}; // Der Einfachheit zuliebe

    // Instruktionsmasken
    static const
    Word form_MultiplyAccumulate =      0b0000'0000'0000'0000'0000'0000'1001'0000;
    static const                    //  ..____'0000'00__'____'____'____'1001'____
    Word mask_MultiplyAccumulate =      0b0000'1111'1100'0000'0000'0000'1111'0000;
    static const
    Word form_MultiplyAccumulateL =     0b0000'0000'1000'0000'0000'0000'1001'0000;
    static const                    //  ..____'0000'1___'____'____'____'1001'____
    Word mask_MultiplyAccumulateL =     0b0000'1111'1000'0000'0000'0000'1111'0000;
    static const
    Word form_BranchAndExchange =       0b0000'0001'0010'1111'1111'1111'0001'0000;
    static const                    //  ..____'0001'0010'1111'1111'1111'0001'____
    Word mask_BranchAndExchange =       0b0000'1111'1111'1111'1111'1111'1111'0000;
    static const
    Word form_SingleDataSwap =          0b0000'0001'0000'0000'0000'0000'1001'0000;
    static const                    //  ..____'0001'0_00'____'____'0000'1001'____
    Word mask_SingleDataSwap =          0b0000'1111'1011'0000'0000'1111'1111'0000;
    static const
    Word form_DataTransferSignHDW =     0b0000'0000'0000'0000'0000'0000'1001'0000; // Data Transfer Signed + Halfword + Double Word
    static const                    //  ..____'000_'____'____'____'____'1__1'____
    Word mask_DataTransferSignHDW =     0b0000'1110'0000'0000'0000'0000'1001'0000;
    static const
    Word form_DataProcAndPSRTransfer =  0b0000'0000'0000'0000'0000'0000'0000'0000; // Falsch??
    static const                    //  ..____'00__'____'____'____'____'____'____     001 oder 00I?
    Word mask_DataProcAndPSRTransfer =  0b0000'1100'0000'0000'0000'0000'0000'0000;
    static const
    Word form_LoadStoreRegUByte =       0b0000'0100'0000'0000'0000'0000'0000'0000; // aka Single Data Transfer
    static const                    //  ..____'01__'____'____'____'____'____'____
    Word mask_LoadStoreRegUByte =       0b0000'1100'0000'0000'0000'0000'0000'0000;
    static const
    Word form_Undefined =               0b0000'0110'0000'0000'0000'0000'0001'0000; // Wie Unterscheiden von LoadStore?
    static const                    //  ..____'011_'____'____'____'____'___1'____
    Word mask_Undefined =               0b0000'1110'0000'0000'0000'0000'0001'0000;
    static const
    Word form_BlockDataTransfer =       0b0000'1000'0000'0000'0000'0000'0000'0000;
    static const                    //  ..____'100_'____'____'____'____'____'____
    Word mask_BlockDataTransfer =       0b0000'1110'0000'0000'0000'0000'0000'0000;
    static const
    Word form_Branch =                  0b0000'1010'0000'0000'0000'0000'0000'0000;
    static const                    //  ..____'101_'____'____'____'____'____'____
    Word mask_Branch =                  0b0000'1110'0000'0000'0000'0000'0000'0000;
    static const
    Word form_CoProcDataTransfer =      0b0000'1100'0000'0000'0000'0000'0000'0000;
    static const                    //  ..____'110_'____'____'____'____'____'____
    Word mask_CoProcDataTransfer =      0b0000'1110'0000'0000'0000'0000'0000'0000;
    static const
    Word form_CoProcDataOperation =     0b0000'1110'0000'0000'0000'0000'0000'0000;
    static const                    //  ..____'1110'____'____'____'____'___0'____
    Word mask_CoProcDataOperation =     0b0000'1111'0000'0000'0000'0000'0001'0000;
    static const
    Word form_CoProcRegTransfer =       0b0000'1110'0000'0000'0000'0000'0001'0000;
    static const                    //  ..____'1110'____'____'____'____'___1'____
    Word mask_CoProcRegTransfer =       0b0000'1111'0000'0000'0000'0000'0001'0000;
    static const
    Word form_SoftwareInterrupt =       0b0000'1111'0000'0000'0000'0000'0000'0000;
    static const                    //  ..____'1111'____'____'____'____'____'____
    Word mask_SoftwareInterrupt =       0b0000'1111'0000'0000'0000'0000'0000'0000;

    // THUMB
    static const
    Word formT_MoveShiftedRegister =        0b0000'0000'0000'0000;
    static const                    //      ..000_'____'____'____
    Word maskT_MoveShiftedRegister =        0b1110'0000'0000'0000; // Achtung
    static const
    Word formT_AddSubtract =                0b0001'1000'0000'0000;
    static const                    //      ..0001'1___'____'____
    Word maskT_AddSubtract =                0b1111'1000'0000'0000;
    static const
    Word formT_MovCmpAddSubImmediate =      0b0010'0000'0000'0000;
    static const                    //      ..001_'____'____'____
    Word maskT_MovCmpAddSubImmediate =      0b1110'0000'0000'0000;
    static const
    Word formT_ALUOperations =              0b0100'0000'0000'0000;
    static const                    //      ..0100'00__'____'____
    Word maskT_ALUOperations =              0b1111'1100'0000'0000;
    static const
    Word formT_HiRegisterOpBX =             0b0100'0100'0000'0000;
    static const                    //      ..0100'01__'____'____
    Word maskT_HiRegisterOpBX =             0b1111'1100'0000'0000;
    static const
    Word formT_PCRelativeLoad =             0b0100'1000'0000'0000;
    static const                    //      ..0100'1___'____'____
    Word maskT_PCRelativeLoad =             0b1111'1000'0000'0000;
    static const
    Word formT_LoadStoreRegOff =            0b0101'0000'0000'0000;
    static const                    //      ..0101'__0_'____'____
    Word maskT_LoadStoreRegOff =            0b1111'0010'0000'0000;
    static const
    Word formT_LoadStoreSignEx =            0b0101'0010'0000'0000;
    static const                    //      ..0101'__1_'____'____
    Word maskT_LoadStoreSignEx =            0b1111'0010'0000'0000;
    static const
    Word formT_LoadStoreImmOff =            0b0110'0000'0000'0000;
    static const                    //      ..011_'____'____'____
    Word maskT_LoadStoreImmOff =            0b1110'0000'0000'0000;
    static const
    Word formT_LoadStoreHalfW =             0b1000'0000'0000'0000;
    static const                    //      ..1000'____'____'____
    Word maskT_LoadStoreHalfW =             0b1111'0000'0000'0000;
    static const
    Word formT_SPRelLoadStore =             0b1001'0000'0000'0000;
    static const                    //      ..1001'____'____'____
    Word maskT_SPRelLoadStore =             0b1111'0000'0000'0000;
    static const
    Word formT_LoadAddress =                0b1010'0000'0000'0000;
    static const                    //      ..1010'____'____'____
    Word maskT_LoadAddress =                0b1111'0000'0000'0000;
    static const
    Word formT_AddOffsetToSP =              0b1011'0000'0000'0000;
    static const                    //      ..1011'0000'____'____
    Word maskT_AddOffsetToSP =              0b1111'1111'0000'0000;
    static const
    Word formT_PushPopRegisters =           0b1011'0100'0000'0000;
    static const                    //      ..1011'_10_'____'____
    Word maskT_PushPopRegisters =           0b1111'0110'0000'0000;
    static const
    Word formT_MultipleLoadStore =          0b1100'0000'0000'0000;
    static const                    //      ..1100'____'____'____
    Word maskT_MultipleLoadStore =          0b1111'0000'0000'0000;
    static const
    Word formT_ConditionalBranch =          0b1101'0000'0000'0000;
    static const                    //      ..1101'____'____'____
    Word maskT_ConditionalBranch =          0b1111'0000'0000'0000;
    static const
    Word formT_SoftwareInterrupt =          0b1101'1111'0000'0000; // Achtung, das vor Conditional Branch!
    static const                    //      ..1101'1111'____'____
    Word maskT_SoftwareInterrupt =          0b1111'1111'0000'0000;
    static const
    Word formT_UnconditionalBranch =        0b1110'0000'0000'0000;
    static const                    //      ..1110'0___'____'____
    Word maskT_UnconditionalBranch =        0b1111'1000'0000'0000;
    static const
    Word formT_LongBranchWithLink =         0b1111'0000'0000'0000;
    static const                    //      ..1111'____'____'____
    Word maskT_LongBranchWithLink =         0b1111'0000'0000'0000;



    enum InstructionType {
        TypeBranchAndExchange,
        TypeSingleDataSwap,
        TypeBranchLink,
        TypeUndefined,
        TypeBlockDataTransfer,
        TypeSofwareInterrupt,
        TypeCoProcDataOperation,
        TypeCoProcDataTransfer,
        TypeCoProcRegTransfer,
        TypeSingleDataTransfer,
        TypeDataTransferSignHDW,
        TypeMultiply,
        TypeMultiplyL,
        TypeDataProc,
        TypePSRTransfer,
        UnimplementedInstruction,
        ThumbMoveShiftedRegister,
        ThumbAddSubtract,
        ThumbMoveCompareAddSubtractImmediate,
        ThumbALUOperations,
        ThumbHiRegisterOperationsBX,
        ThumbPCRelativeLoad,
        ThumbLoadStoreWithRegisterOff,
        ThumbLoadStoreSignExtendedBHW,
        ThumbLoadStoreWithImmOffset,
        ThumbLoadStoreHalfWord,
        ThumbSPRelativeLoadStore,
        ThumbLoadAddress,
        ThumbAddOffsetToSP,
        ThumbPushPopRegisters,
        ThumbMultipleLoadStore,
        ThumbConditionalBranch,
        ThumbSoftwareInterrupt,
        ThumbUnconditionalBranch,
        ThumbLongBranchWithLink,
    };

    struct InstructionInfo {
        InstructionType type;
        Word code;
    };

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
