#include "arm7tdmi.h"
#include "framework/stringlib.h"
#include "gba/test/logging.h"
#include <bit>
#include <bitset>
#include <fstream>
#include <string>
#include <iostream>
#include "../test/logging.h"
extern "C" {
#include <armdisasm.h>
}

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
    // if(fits){
    //     bool P = code & (1u << 24);
    //     bool I = code & (1u << 22);
    //     if(!P){
    //         // Bit 21 MUSS 0 sein
    //         bool check = code & (1u << 21);
    //         if(check) return false;
    //     }
    //     if(!I){
    //         // Bits 11-8 müssen 0 sein
    //         if((code & (0b1111 << 8)) != 0) return false;
    //     }
    //     if((code & (0b11 << 5)) != 0){ // Opcode (bits 5-6) darf nicht 0 sein
    //         return true;
    //     }
    // }
    return fits;
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
    // if(fits){
    //     bool I = code & (1u << 25);
    //     Word opcode = ((0b1111u << 21) & code) >> 21;
    //     bool S = code & (1u << 20);
    //     Word Rn = code & (0b1111 << 16);
    //     Word Rd = code & (0b1111 << 12);
    //     if(opcode >= 0x8 && opcode <= 0xB){
    //         if(!S) return false;
    //         if(Rd != 0b0000 && Rd != 0b1111) return false;
    //     }
    //     if((opcode == 0xD || opcode == 0xF) && Rn != 0) return false;
    //     if(!I){
    //         bool R = code & (1u << 4);
    //         if(R){
    //             if(code & (1u << 7) > 0) return false;
    //         }
    //     }
    //     return true;
    // }
    return fits;
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
                if((code & (255u << 4)) > 0) return false;
            }
        }
        else{ // MRS
            if((code & (0b1111 << 16)) >> 16 != 0b1111) return false; //mus 0b1111 sein
            if((code & 0b11111111111) > 0) return false; // muss 0 sein
        }
        return true;
    }
    return fits;
}

static inline bool isThumbMoveShiftedRegister(HalfWord code){
    auto masked = code & maskT_MoveShiftedRegister;
    return (masked == formT_MoveShiftedRegister) && ((code & (0b11u << 11)) != (0b11u << 11)); // AddSub
}
static inline bool isThumbAddSubtract(HalfWord code){
    auto masked = code & maskT_AddSubtract;
    return masked == formT_AddSubtract;
}
static inline bool isThumbMoveCompareAddSubtractImmediate(HalfWord code){
    auto masked = code & maskT_MovCmpAddSubImmediate;
    return masked == formT_MovCmpAddSubImmediate;
}
static inline bool isThumbALUOperations(HalfWord code){
    auto masked = code & maskT_ALUOperations;
    return masked == formT_ALUOperations;
}
static inline bool isThumbHiRegisterOperationsBX(HalfWord code){
    auto masked = code & maskT_HiRegisterOpBX;
    return masked == formT_HiRegisterOpBX;
}
static inline bool isThumbPCRelativeLoad(HalfWord code){
    auto masked = code & maskT_PCRelativeLoad;
    return masked == formT_PCRelativeLoad;
}
static inline bool isThumbLoadStoreWithRegisterOff(HalfWord code){
    auto masked = code & maskT_LoadStoreRegOff;
    return masked == formT_LoadStoreRegOff;
}
static inline bool isThumbLoadStoreSignExtendedBHW(HalfWord code){
    auto masked = code & maskT_LoadStoreSignEx;
    return masked == formT_LoadStoreSignEx;
}
static inline bool isThumbLoadStoreWithImmOffset(HalfWord code){
    auto masked = code & maskT_LoadStoreImmOff;
    return masked == formT_LoadStoreImmOff;
}
static inline bool isThumbLoadStoreHalfWord(HalfWord code){
    auto masked = code & maskT_LoadStoreHalfW;
    return masked == formT_LoadStoreHalfW;
}
static inline bool isThumbSPRelativeLoadStore(HalfWord code){
    auto masked = code & maskT_SPRelLoadStore;
    return masked == formT_SPRelLoadStore;
}
static inline bool isThumbLoadAddress(HalfWord code){
    auto masked = code & maskT_LoadAddress;
    return masked == formT_LoadAddress;
}
static inline bool isThumbAddOffsetToSP(HalfWord code){
    auto masked = code & maskT_AddOffsetToSP;
    return masked == formT_AddOffsetToSP;
}
static inline bool isThumbPushPopRegisters(HalfWord code){
    auto masked = code & maskT_PushPopRegisters;
    return masked == formT_PushPopRegisters;
}
static inline bool isThumbMultipleLoadStore(HalfWord code){
    auto masked = code & maskT_MultipleLoadStore;
    return masked == formT_MultipleLoadStore;
}
static inline bool isThumbConditionalBranch(HalfWord code){
    auto masked = code & maskT_ConditionalBranch;
    return (masked == formT_ConditionalBranch) && ((code & 0x0F00) != 0x0F00); // SWI
}
static inline bool isThumbSoftwareInterrupt(HalfWord code){
    auto masked = code & maskT_SoftwareInterrupt;
    return masked == formT_SoftwareInterrupt;
}
static inline bool isThumbUnconditionalBranch(HalfWord code){
    auto masked = code & maskT_UnconditionalBranch;
    return masked == formT_UnconditionalBranch;
}
static inline bool isThumbLongBranchWithLink(HalfWord code){
    auto masked = code & maskT_LongBranchWithLink;
    return masked == formT_LongBranchWithLink;
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
        case ThumbMoveShiftedRegister:
            s = "ThumbMoveShiftedRegister";
            break;
        case ThumbAddSubtract:
            s = "ThumbAddSubtract";
            break;
        case ThumbMoveCompareAddSubtractImmediate:
            s = "ThumbMoveCompareAddSubtractImmediate";
            break;
        case ThumbALUOperations:
            s = "ThumbALUOperations";
            break;
        case ThumbHiRegisterOperationsBX:
            s = "ThumbHiRegisterOperationsBX";
            break;
        case ThumbPCRelativeLoad:
            s = "ThumbPCRelativeLoad";
            break;
        case ThumbLoadStoreWithRegisterOff:
            s = "ThumbLoadStoreWithRegisterOff";
            break;
        case ThumbLoadStoreSignExtendedBHW:
            s = "ThumbLoadStoreSignExtendedBHW";
            break;
        case ThumbLoadStoreWithImmOffset:
            s = "ThumbLoadStoreWithImmOffset";
            break;
        case ThumbLoadStoreHalfWord:
            s = "ThumbLoadStoreHalfWord";
            break;
        case ThumbSPRelativeLoadStore:
            s = "ThumbSPRelativeLoadStore";
            break;
        case ThumbLoadAddress:
            s = "ThumbLoadAddress";
            break;
        case ThumbAddOffsetToSP:
            s = "ThumbAddOffsetToSP";
            break;
        case ThumbPushPopRegisters:
            s = "ThumbPushPopRegisters";
            break;
        case ThumbMultipleLoadStore:
            s = "ThumbMultipleLoadStore";
            break;
        case ThumbConditionalBranch:
            s = "ThumbConditionalBranch";
            break;
        case ThumbSoftwareInterrupt:
            s = "ThumbSoftwareInterrupt";
            break;
        case ThumbUnconditionalBranch:
            s = "ThumbUnconditionalBranch";
            break;
        case ThumbLongBranchWithLink:
            s = "ThumbLongBranchWithLink";
            break;
    }
    return s;
}

bool gba::CPU::executeInstruction()
{
    // auto catastrophicExit = [&](int num){
    //     std::cout << "KATASTROPHE: Fall " << num << " ist eingetreten!\n";
    //     // throw "ERROR";
    //     std::cout << "Adresse: " << getHex0x(_R15_PC, 8) << "\n";
    //     std::cout << getDisassembly(pipelineDecoded.value().code) << "\n";
    //     std::cout << std::bitset<32>(pipelineDecoded.value().code) << "\n";
    //     bus.lock()->setHalt();
    // };
    if(this->pipelineDecoded.has_value()){
        InstructionInfo info = this->pipelineDecoded.value();
        Condition cond = (Condition)((info.code & 0xF0000000) >> 28);

        // auto iInfo = info;

        // if(iInfo.type == gba::TypeBlockDataTransfer){
        //     if(iInfo.code & (1u << 22)) catastrophicExit(1); // S gesetzt
        //     if(((iInfo.code & (0xFu << 16)) >> 16) == 15) catastrophicExit(2); // R15 als Basisregister ("R15 should not be used as the base register in any LDM or STM instruction")
        // }
        // if(iInfo.type == gba::TypePSRTransfer){
        //     if((iInfo.code & 0xFFF) == 0 && (((iInfo.code & (0b111111 << 16)) >> 16) == 0xF) && (((iInfo.code & (0b11111 << 23)) >> 23) == 0b10)){ // MRS
        //         if(((iInfo.code & (0xFu << 12)) >> 12) == 15) catastrophicExit(3); // "You must not specify R15 as destination Register"
        //     }
        // }
        // if(iInfo.type == gba::TypeMultiply){ // R15 als Operand ist illegal
        //     if((iInfo.code & (0xFu)) == R15) catastrophicExit(4);
        //     if(((iInfo.code & (0xFu << 8)) >> 8) == R15) catastrophicExit(5);
        //     if(((iInfo.code & (0xFu << 12)) >> 12) == R15) catastrophicExit(6);
        // }
        // if(iInfo.type == gba::TypeMultiplyL){ // R15 als Operand oder Destination ist illegal
        //     if((iInfo.code & (0xFu)) == R15) catastrophicExit(7);
        //     if(((iInfo.code & (0xFu << 8)) >> 8) == R15) catastrophicExit(8);
        //     if(((iInfo.code & (0xFu << 12)) >> 12) == R15) catastrophicExit(9);
        //     if(((iInfo.code & (0xFu << 16)) >> 16) == R15) catastrophicExit(10);
        // }
        // if(iInfo.type == gba::TypeSingleDataSwap){ // "Do not use R15 as Parameters"
        //     if((iInfo.code & 0xF) == R15) catastrophicExit(11);
        //     if(((iInfo.code & 0xF000) >> 12) == R15) catastrophicExit(12);
        //     if(((iInfo.code & 0xF0000) >> 16) == R15) catastrophicExit(13);
        // }

        // std::string line;
        // std::getline(openlara, line);
        // std::string token = "0x"+line.substr(0, line.find(" "));
        // int offset = state() == ARM ? 8 : 4;
        // if(token != getHex0x(_R15_PC - offset, 8)){
        //     std::cout << "PC weicht ab.\n";
        //     std::cout << "Ich:   \t" << getHex0x(_R15_PC - offset, 8) << "\n";
        //     std::cout << "mesen: \t" << token << "\n";
        //     bus.lock()->setHalt();
        // }


        if(this->state() == ARM){
            // ARMSTATE s;
            // disasm_init(&s, DISASM_ADDRESS | DISASM_INSTR);
            // s.address = _R15_PC;
            // disasm_arm(&s, info.code);
            // std::cout << getHex(_R15_PC-8, 8) << "\n";
            // std::cout << s.text << "\n";
            // disasm_cleanup(&s);
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
                    default:
                        std::cout << "Unimplementierte Instruktion ausgeführt" << std::endl;
                        bus.lock()->setHalt();
                        // throw "error";
                        return false;
                }
            }
            else return false;
        }
        else{ // THUMB
            switch(info.type){
                case ThumbMoveShiftedRegister:
                    return this->executeThumbMoveShiftedRegister(info.code);
                case ThumbAddSubtract:
                    return this->executeThumbAddSubtract(info.code);
                case ThumbMoveCompareAddSubtractImmediate:
                    return this->executeThumbMoveCompareAddSubtractImmediate(info.code);
                case ThumbALUOperations:
                    return this->executeThumbALUOperations(info.code);
                case ThumbHiRegisterOperationsBX:
                    return this->executeThumbHiRegisterOperationsBX(info.code);
                case ThumbPCRelativeLoad:
                    return this->executeThumbPCRelativeLoad(info.code);
                case ThumbLoadStoreWithRegisterOff:
                    return this->executeThumbLoadStoreWithRegisterOff(info.code);
                case ThumbLoadStoreSignExtendedBHW:
                    return this->executeThumbLoadStoreSignExtendedBHW(info.code);
                case ThumbLoadStoreWithImmOffset:
                    return this->executeThumbLoadStoreWithImmOffset(info.code);
                case ThumbLoadStoreHalfWord:
                    return this->executeThumbLoadStoreHalfWord(info.code);
                case ThumbSPRelativeLoadStore:
                    return this->executeThumbSPRelativeLoadStore(info.code);
                case ThumbLoadAddress:
                    return this->executeThumbLoadAddress(info.code);
                case ThumbAddOffsetToSP:
                    return this->executeThumbAddOffsetToSP(info.code);
                case ThumbPushPopRegisters:
                    return this->executeThumbPushPopRegisters(info.code);
                case ThumbMultipleLoadStore:
                    return this->executeThumbMultipleLoadStore(info.code);
                case ThumbConditionalBranch:
                    return this->executeThumbConditionalBranch(info.code);
                case ThumbSoftwareInterrupt:
                    return this->executeThumbSoftwareInterrupt(info.code);
                case ThumbUnconditionalBranch:
                    return this->executeThumbUnconditionalBranch(info.code);
                case ThumbLongBranchWithLink:
                    return this->executeThumbLongBranchWithLink(info.code);
                default:
                    std::cout << "Unimplementierte Instruktion ausgeführt" << std::endl;
                    // throw "error";
                    bus.lock()->setHalt();
                    return false;
            }
        }
    }
    return false;
}

InstructionInfo gba::CPU::decodeInstruction(Word code)
{
    if(this->state() == ARM){
        return decodeInstructionARM(code);
    }
    else{
        return decodeInstructionTHUMB(code);
    }
}

InstructionInfo gba::CPU::decodeInstructionARM(Word code)
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
    
    if(isMultiplyAccumulate(code))
        return InstructionInfo{.type = TypeMultiply, .code = code};
    if(isMultiplyAccumulateL(code))
        return InstructionInfo{.type = TypeMultiplyL, .code = code};
    
    if(isDataTransferSignHDW(code))
        return InstructionInfo{.type = TypeDataTransferSignHDW, .code = code};
    

    
    if(isPSRTransfer(code))
        return InstructionInfo{.type = TypePSRTransfer, .code = code};

    if(isDataProc(code))
        return InstructionInfo{.type = TypeDataProc, .code = code};


    return InstructionInfo{.type = UnimplementedInstruction, .code = code};
}

InstructionInfo gba::CPU::decodeInstructionTHUMB(Word code)
{
    if(isThumbAddSubtract(code))
        return InstructionInfo{.type = ThumbAddSubtract, .code = code};

    if(isThumbMoveShiftedRegister(code))
        return InstructionInfo{.type = ThumbMoveShiftedRegister, .code = code};
        
    if(isThumbMoveCompareAddSubtractImmediate(code))
        return InstructionInfo{.type = ThumbMoveCompareAddSubtractImmediate, .code = code};

    if(isThumbALUOperations(code))
        return InstructionInfo{.type = ThumbALUOperations, .code = code};

    if(isThumbHiRegisterOperationsBX(code))
        return InstructionInfo{.type = ThumbHiRegisterOperationsBX, .code = code};

    if(isThumbPCRelativeLoad(code))
        return InstructionInfo{.type = ThumbPCRelativeLoad, .code = code};

    if(isThumbLoadStoreWithRegisterOff(code))
        return InstructionInfo{.type = ThumbLoadStoreWithRegisterOff, .code = code};

    if(isThumbLoadStoreSignExtendedBHW(code))
        return InstructionInfo{.type = ThumbLoadStoreSignExtendedBHW, .code = code};

    if(isThumbLoadStoreWithImmOffset(code))
        return InstructionInfo{.type = ThumbLoadStoreWithImmOffset, .code = code};

    if(isThumbLoadStoreHalfWord(code))
        return InstructionInfo{.type = ThumbLoadStoreHalfWord, .code = code};

    if(isThumbSPRelativeLoadStore(code))
        return InstructionInfo{.type = ThumbSPRelativeLoadStore, .code = code};

    if(isThumbLoadAddress(code))
        return InstructionInfo{.type = ThumbLoadAddress, .code = code};

    if(isThumbAddOffsetToSP(code))
        return InstructionInfo{.type = ThumbAddOffsetToSP, .code = code};

    if(isThumbPushPopRegisters(code))
        return InstructionInfo{.type = ThumbPushPopRegisters, .code = code};

    if(isThumbMultipleLoadStore(code))
        return InstructionInfo{.type = ThumbMultipleLoadStore, .code = code};
        
    if(isThumbSoftwareInterrupt(code))
        return InstructionInfo{.type = ThumbSoftwareInterrupt, .code = code};

    if(isThumbConditionalBranch(code))
        return InstructionInfo{.type = ThumbConditionalBranch, .code = code};

    if(isThumbUnconditionalBranch(code))
        return InstructionInfo{.type = ThumbUnconditionalBranch, .code = code};

    if(isThumbLongBranchWithLink(code))
        return InstructionInfo{.type = ThumbLongBranchWithLink, .code = code};

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

    _CPSR.state.mode_bits = 31;

    _R13_SP = 0x03007F00;
    _R13_SVC = 0x03007FE0;
    _R13_irq = 0x03007FA0;
    _R14_L_R = 0x08000000; 
    _R15_PC = 0x08000000;
}

bool gba::CPU::checkCondition(Condition c) const
{
    switch(c){
        case EQ: return std::bit_cast<StatusRegister>(*this->registerMap[mode()][CPSR]).state.Z == 1;
        case NE: return std::bit_cast<StatusRegister>(*this->registerMap[mode()][CPSR]).state.Z == 0;
        case CS: return std::bit_cast<StatusRegister>(*this->registerMap[mode()][CPSR]).state.C == 1;
        case CC: return std::bit_cast<StatusRegister>(*this->registerMap[mode()][CPSR]).state.C == 0;
        case MI: return std::bit_cast<StatusRegister>(*this->registerMap[mode()][CPSR]).state.N == 1;
        case PL: return std::bit_cast<StatusRegister>(*this->registerMap[mode()][CPSR]).state.N == 0;
        case VS: return std::bit_cast<StatusRegister>(*this->registerMap[mode()][CPSR]).state.V == 1;
        case VC: return std::bit_cast<StatusRegister>(*this->registerMap[mode()][CPSR]).state.V == 0;
        case HI: {
                auto status = std::bit_cast<StatusRegister>(*this->registerMap[mode()][CPSR]);
                return status.state.C == 1 && status.state.Z == 0;
            }
        case LS: {
                auto status = std::bit_cast<StatusRegister>(*this->registerMap[mode()][CPSR]);
                return status.state.C == 0 || status.state.Z == 1;
            }
        case GE: {
                auto status = std::bit_cast<StatusRegister>(*this->registerMap[mode()][CPSR]);
                return status.state.N == status.state.V;
            }
        case LT: {
                auto status = std::bit_cast<StatusRegister>(*this->registerMap[mode()][CPSR]);
                return status.state.N != status.state.V;
            }
        case GT: {
                auto status = std::bit_cast<StatusRegister>(*this->registerMap[mode()][CPSR]);
                return status.state.Z == 0 && (status.state.N == status.state.V);
            }
        case LE: {
                auto status = std::bit_cast<StatusRegister>(*this->registerMap[mode()][CPSR]);
                return status.state.Z == 1 || (status.state.N != status.state.V);
            }
        case AL: return true;
        case NV: return false;
    }
    return false;
}

std::string CPU::getDisassembly(Word code){
    ARMSTATE s;
    auto state = getRegisterState();
    disasm_init(&s, 0);
    int mode = ((state.CPSR >> 5) & 1u) ? 0 : 1;
    s.arm_mode = mode;
    if(mode == 1){
        disasm_arm(&s, code);
    }
    else{
        disasm_thumb(&s, uint16_t(code), uint16_t(code >> 16));
    }
    std::string text = s.text;
    disasm_cleanup(&s);
    return text;
}
