#include "arm7tdmi.h"
#include <bit>
#include <string>
#include <iostream>

using namespace gba;
/*
ARM Binary Opcode Format (Arm11 / Arm9 Instruktionen entfernt) GBATEK
  |..3 ..................2 ..................1 ..................0|
  |1_0_9_8_7_6_5_4_3_2_1_0_9_8_7_6_5_4_3_2_1_0_9_8_7_6_5_4_3_2_1_0|
  |_Cond__|0_0_0|___Op__|S|__Rn___|__Rd___|__Shift__|Typ|0|__Rm___| DataProc
  |_Cond__|0_0_0|___Op__|S|__Rn___|__Rd___|__Rs___|0|Typ|1|__Rm___| DataProc
  |_Cond__|0_0_1|___Op__|S|__Rn___|__Rd___|_Shift_|___Immediate___| DataProc
  |_Cond__|0_0_1_1_0|P|1|0|_Field_|__Rd___|_Shift_|___Immediate___| PSR Imm
  |_Cond__|0_0_0_1_0|P|L|0|_Field_|__Rd___|0_0_0_0|0_0_0_0|__Rm___| PSR Reg
  |_Cond__|0_0_0_0_0_0|A|S|__Rd___|__Rn___|__Rs___|1_0_0_1|__Rm___| Multiply
  |_Cond__|0_0_0_0_1|U|A|S|_RdHi__|_RdLo__|__Rs___|1_0_0_1|__Rm___| MulLong
  |_Cond__|0_0_0|P|U|0|W|L|__Rn___|__Rd___|0_0_0_0|1|S|H|1|__Rm___| TransReg10
  |_Cond__|0_0_0|P|U|1|W|L|__Rn___|__Rd___|OffsetH|1|S|H|1|OffsetL| TransImm10
  |_Cond__|0_1_0|P|U|B|W|L|__Rn___|__Rd___|_________Offset________| TransImm9
  |_Cond__|0_1_1|P|U|B|W|L|__Rn___|__Rd___|__Shift__|Typ|0|__Rm___| TransReg9
  |_Cond__|0_1_1|________________xxx____________________|1|__xxx__| Undefined
  |_Cond__|1_0_0|P|U|S|W|L|__Rn___|__________Register_List________| BlockTrans
  |_Cond__|1_1_0|P|U|N|W|L|__Rn___|__CRd__|__CP#__|____Offset_____| CoDataTrans
  |_Cond__|1_1_1_0|_CPopc_|__CRn__|__CRd__|__CP#__|_CP__|0|__CRm__| CoDataOp
  |_Cond__|1_1_1_0|CPopc|L|__CRn__|__Rd___|__CP#__|_CP__|1|__CRm__| CoRegTrans
  
  |_Cond__|0_0_0_1_0|B|0_0|__Rn___|__Rd___|0_0_0_0|1_0_0_1|__Rm___| TransSwp12 (Single Data Swap)
  |_Cond__|1_0_1|L|___________________Offset______________________| B,BL,BLX (Branch)
  |_Cond__|1_1_1_1|_____________Ignored_by_Processor______________| SWI
  |_Cond__|0_0_0_1_0_0_1_0_1_1_1_1_1_1_1_1_1_1_1_1|0_0|L|1|__Rn___| BX,BLX (Branch Exchange / Link)
*/

static inline bool isBranchAndExchange(Word code)
{
    auto masked = code & mask_BranchAndExchange;
    return masked == form_BranchAndExchange;
}

static inline bool isSingleDataSwap(Word code)
{
    auto masked = code & mask_SingleDataSwap;
    return masked == form_SingleDataSwap;
}

static inline bool isUndefined(Word code)
{
    auto masked = code & mask_Undefined;
    return masked == form_Undefined;
}

static inline bool isBranchLink(Word code)
{
    auto masked = code & mask_Branch;
    return masked == form_Branch;
}

static inline bool isBlockDataTransfer(Word code)
{
    auto masked = code & mask_BlockDataTransfer;
    return masked == form_BlockDataTransfer;
}

static inline bool isSoftwareInterrupt(Word code)
{
    auto masked = code & mask_SoftwareInterrupt;
    return masked == form_SoftwareInterrupt;
}

static inline bool isCoProcDataTransfer(Word code)
{
    auto masked = code & mask_CoProcDataTransfer;
    return masked == form_CoProcDataTransfer;
}

static inline bool isCoProcDataOperation(Word code)
{
    auto masked = code & mask_CoProcDataOperation;
    return masked == form_CoProcDataOperation;
}

static inline bool isCoProcRegTransfer(Word code)
{
    auto masked = code & mask_CoProcRegTransfer;
    return masked == form_CoProcRegTransfer;
}

static inline bool isSingleDataTransfer(Word code)
{
    // Hier fehlt vielleicht was?
    auto masked = code & mask_LoadStoreRegUByte;
    auto fits = masked == form_LoadStoreRegUByte;
    if(fits){
        bool I = code & (1u << 25);
        if(I){
            // Bit 4 muss 0 sein, sonst Undefined
            return (code & (1u << 4)) == 0;
        }
        else{
            return true;
        }
    }
    return false;
}

static inline bool isDataTransferSignHDW(Word code)
{
    auto masked = code & mask_DataTransferSignHDW;
    auto fits = masked == form_DataTransferSignHDW;
    if(fits){
        bool P = code & (1u << 24);
        bool I = code & (1u << 22);
        bool L = code & (1u << 20);
        if(!P){
            // Bit 21 MUSS 0 sein
            bool check = code & (1u << 21);
            if(check) return false;
        }
        if(!I){
            // Bits 11-8 müssen 0 sein
            if(code & (0b1111 << 8) != 0) return false;
        }
        if(code & (0b11 << 5) != 0){ // Opcode (bits 5-6) darf nicht 0 sein
            return true;
        }
    }
    return false;
}

static inline bool isMultiplyAccumulate(Word code)
{
    auto masked = code & mask_MultiplyAccumulate;
    return masked == form_MultiplyAccumulate;
}

static inline bool isMultiplyAccumulateL(Word code)
{
    auto masked = code & mask_MultiplyAccumulateL;
    return masked == form_MultiplyAccumulateL;
}

static inline bool isDataProc(Word code)
{
    auto masked = code & mask_DataProcAndPSRTransfer;
    auto fits = masked == form_DataProcAndPSRTransfer;
    if(fits){
        bool I = code & (1u << 25);
        Word opcode = ((0b1111u << 21) & code) >> 21;
        bool S = code & (1u << 20);
        Word Rn = code & (0b1111 << 16);
        Word Rd = code & (0b1111 << 12);
        if(opcode >= 0x8 && opcode <= 0xB){
            if(!S) return false;
            if(Rd != 0b0000 && Rd != 0b1111) return false;
        }
        if((opcode == 0xD || opcode == 0xF) && Rn != 0) return false;
        if(!I){
            bool R = code & (1u << 4);
            if(R){
                if(code & (1u << 7) > 0) return false;
            }
        }
        return true;
    }
    return false;
}

static inline bool isPSRTransfer(Word code)
{
    auto masked = code & mask_DataProcAndPSRTransfer;
    auto fits = masked == form_DataProcAndPSRTransfer;
    if(fits){
        bool I = code & (1u << 25);
        if((code & (0b11u << 23)) >> 23 != 0b10) return false; // muss 0b10 sein
        bool opcode = (1u << 21) & code;
        if((code & (1u << 20)) > 0) return false; // muss 0 sein
        if(opcode){ // MSR

            if(!I){
                if(code & (255u << 4) > 0) return false;
            }
        }
        else{ // MRS
            if((code & (0b1111 << 16)) >> 16 != 0b1111) return false; //mus 0b1111 sein
            if(code & 0b11111111111 > 0) return false; // muss 0 sein
        }
        return true;
    }
    return false;
}

std::string gba::CPU::printInstructionType(InstructionType t){
    std::string s = "";
    switch(t){
        case TypeBranchAndExchange:
            s = "BranchExchange";
            break;
        case TypeSingleDataSwap:
            s = "SingleDataSwap";
            break;
        case TypeBranchLink:
            s = "BranchLink";
            break;
        case TypeUndefined:
            s = "Undefined";
            break;
        case TypeBlockDataTransfer:
            s = "BlockDataTransfer";
            break;
        case TypeSofwareInterrupt:
            s = "SoftwareInterrupt";
            break;
        case TypeCoProcDataOperation:
            s = "CoProcDataOp";
            break;
        case TypeCoProcDataTransfer:
            s = "CoProcDataTransfer";
            break;
        case TypeCoProcRegTransfer:
            s = "CoProcRegTransfer";
            break;
        case TypeSingleDataTransfer:
            s = "SingleDataTransfer";
            break;
        case TypeDataTransferSignHDW:
            s = "DataTransferSignHDW";
            break;
        case TypeMultiply:
            s = "Multiply";
            break;
        case TypeMultiplyL:
            s = "MultiplyL";
            break;
        case TypeDataProc:
            s = "DataProc";
            break;
        case TypePSRTransfer:
            s = "PSRTransfer";
            break;
        case UnimplementedInstruction:
            s = "Error: Unimplemented!";
            break;
    }
    return s;
}

bool gba::CPU::executeInstruction()
{
    if(this->pipelineDecoded.has_value()){
        InstructionInfo info = this->pipelineDecoded.value();
        Condition cond = (Condition)((info.code & 0xF0000000) >> 28);
        if(this->checkCondition(cond)){
            switch(info.type){
                case TypeBranchAndExchange:
                    return this->executeBranchExchange(info.code);
                case TypeSingleDataSwap:
                    return this->executeSingleDataSwap(info.code);
                case TypeBranchLink:
                    return this->executeBranchLink(info.code);
                case TypeUndefined:
                    return this->executeUndefined(info.code);
                    return false;
                case TypeBlockDataTransfer:
                    return this->executeBlockDataTransfer(info.code);
                case TypeSofwareInterrupt:
                    return this->executeSoftwareInterrupt(info.code);
                case TypeCoProcDataOperation:
                    return this->executeCoProcDataOperation(info.code);
                case TypeCoProcDataTransfer:
                    return this->executeCoProcDataTransfer(info.code);
                case TypeCoProcRegTransfer:
                    return this->executeCoProcRegTransfer(info.code);
                case TypeSingleDataTransfer:
                    return this->executeSingleDataTransfer(info.code);
                case TypeDataTransferSignHDW:
                    return this->executeDataTransferSignHDW(info.code);
                case TypeMultiply:
                    return this->executeMultiply(info.code);
                case TypeMultiplyL:
                    return this->executeMultiplyLong(info.code);
                case TypeDataProc:
                    return this->executeDataProc(info.code);
                case TypePSRTransfer:
                    return this->executePSRTransfer(info.code);
                case UnimplementedInstruction:
                    std::cout << "Unimplementierte Instruktion ausgeführt" << std::endl;
                    return false;
            }
        }
        else return false;
    }
    else return false;
}

InstructionInfo gba::CPU::decodeInstruction(Word code)
{

    // Leicht zu dekodierende Instruktionen zuerst
    if(isBranchAndExchange(code))
        return InstructionInfo{.type = TypeBranchAndExchange, .code = code};

    if(isSingleDataSwap(code))
        return InstructionInfo{.type = TypeSingleDataSwap, .code = code};

    if(isSoftwareInterrupt(code))
        return InstructionInfo{.type = TypeSofwareInterrupt, .code = code};

    if(isBranchLink(code))
        return InstructionInfo{.type = TypeBranchLink, .code = code};
    
    if(isBlockDataTransfer(code))
        return InstructionInfo{.type = TypeBlockDataTransfer, .code = code};


    // Brauche ich nicht?
        if(isCoProcDataTransfer(code))
            return InstructionInfo{.type = TypeCoProcDataTransfer, .code = code};

        if(isCoProcDataOperation(code))
            return InstructionInfo{.type = TypeCoProcDataOperation, .code = code};

        if(isCoProcRegTransfer(code))
            return InstructionInfo{.type = TypeCoProcRegTransfer, .code = code};
    //...

    if(isSingleDataTransfer(code))
        return InstructionInfo{.type = TypeSingleDataTransfer, .code = code};
    
    if(isUndefined(code))
        return InstructionInfo{.type = TypeUndefined, .code = code};
    
    if(isDataTransferSignHDW(code))
        return InstructionInfo{.type = TypeDataTransferSignHDW, .code = code};
    
    if(isMultiplyAccumulate(code))
        return InstructionInfo{.type = TypeMultiply, .code = code};

    if(isMultiplyAccumulateL(code))
        return InstructionInfo{.type = TypeMultiplyL, .code = code};
    
    if(isDataProc(code))
        return InstructionInfo{.type = TypeDataProc, .code = code};

    if(isPSRTransfer(code))
        return InstructionInfo{.type = TypePSRTransfer, .code = code};

    return InstructionInfo{.type = UnimplementedInstruction, .code = code};
}

gba::CPU::CPU()
{
    // Init Status
    _CPSR.raw = 0;
    _SPSR_fiq.raw = 0;
    _SPSR_SVC.raw = 0;
    _SPSR_abt.raw = 0;
    _SPSR_irq.raw = 0;
    _SPSR_und.raw = 0;
}

bool gba::CPU::checkCondition(Condition c) const
{
    switch(c){
        case EQ: return std::bit_cast<StatusRegister>(*this->registerMap[mode][CPSR]).Z == 1;
        case NE: return std::bit_cast<StatusRegister>(*this->registerMap[mode][CPSR]).Z == 0;
        case CS: return std::bit_cast<StatusRegister>(*this->registerMap[mode][CPSR]).C == 1;
        case CC: return std::bit_cast<StatusRegister>(*this->registerMap[mode][CPSR]).C == 0;
        case MI: return std::bit_cast<StatusRegister>(*this->registerMap[mode][CPSR]).N == 1;
        case PL: return std::bit_cast<StatusRegister>(*this->registerMap[mode][CPSR]).N == 0;
        case VS: return std::bit_cast<StatusRegister>(*this->registerMap[mode][CPSR]).V == 1;
        case VC: return std::bit_cast<StatusRegister>(*this->registerMap[mode][CPSR]).V == 0;
        case HI: {
                auto status = std::bit_cast<StatusRegister>(*this->registerMap[mode][CPSR]);
                return status.C == 1 && status.Z == 0;
            }
        case LS: {
                auto status = std::bit_cast<StatusRegister>(*this->registerMap[mode][CPSR]);
                return status.C == 0 || status.Z == 1;
            }
        case GE: {
                auto status = std::bit_cast<StatusRegister>(*this->registerMap[mode][CPSR]);
                return status.N == status.V;
            }
        case LT: {
                auto status = std::bit_cast<StatusRegister>(*this->registerMap[mode][CPSR]);
                return status.N != status.V;
            }
        case GT: {
                auto status = std::bit_cast<StatusRegister>(*this->registerMap[mode][CPSR]);
                return status.Z == 0 && (status.N == status.V);
            }
        case LE: {
                auto status = std::bit_cast<StatusRegister>(*this->registerMap[mode][CPSR]);
                return status.Z == 1 || (status.N != status.V);
            }
        case AL: return true;
        case NV: return false;
    }
    return false;
}
