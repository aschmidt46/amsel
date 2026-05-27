#include "arm7tdmi.h"

using namespace gba;

bool gba::CPU::executeThumbMoveShiftedRegister(Word instruction)
{
    Word tshift = instruction & (0b11u << 11);
    Word offset = instruction & (0b11111u << 6);
    Word Rd = instruction & 0b111;
    Word Rs = (instruction & (0b111u << 3)) >> 3;
    //                                               ---S----
    Word arm = (offset << 7) | (tshift << 5) | Rs | (1u << 20) | (Rd << 12);
    return executeDataProc(arm);
}

bool gba::CPU::executeThumbAddSubtract(Word instruction)
{
    bool I = instruction & (1u << 10);
    Word opcode = (instruction & (1u << 9)) ? 0b0010 : 0b0100;
    Word Rd = instruction & 0b111;
    Word Rs = (instruction & (0b111 << 3)) >> 3;
    Word Rn = (instruction & (0b111 << 6)) >> 6;
    Word arm = Rn | (Rd << 12) | (Rs << 16) | (1u << 20) | (opcode << 21) | (I << 25);
    return executeDataProc(arm);
}

bool gba::CPU::executeThumbMoveCompareAddSubtractImmediate(Word instruction)
{
    Word imm = instruction & 0xFF;
    Word Rd = (instruction & (0b111 << 8)) >> 8;
    Word opcode = (instruction & (0b11 << 11)) >> 11;
    if(opcode == 0){
        opcode = 0b1101; // mov
    }
    else if(opcode == 1){
        opcode = 0b1010; //cmp
    }
    else if(opcode == 2){
        opcode = 0b0100; //add
    }
    else{
        opcode = 0b0010; // sub
    }
    //                                            S                             Imm
    Word arm = imm | (Rd << 12) | (Rd << 16) | (1u << 20) | (opcode << 21) | (1u << 25);
    return executeDataProc(arm);
}

bool gba::CPU::executeThumbALUOperations(Word instruction)
{
    Word opcode = (instruction & (0xFu << 6)) >> 6;
    Word Rs = (instruction & (0b111u << 3)) >> 3;
    Word Rd = instruction & 0b111;

    if((opcode >= 2 && opcode <= 0b0100) || opcode == 0b0111 || opcode == 0b1001 || opcode == 0b1101){
        Word lsl, lsr, asr, ror;
        switch(opcode){
            case 0b0010:
                lsl = (1 << 4) | (Rs << 8);
                return executeDataProc(     (1u << 20) | (Rd << 12) | lsl | (Rd) | (0b1101u << 21)   );
            case 0b0011:
                lsr = (1 << 4) | (1 << 5) | (Rs << 8);
                return executeDataProc(     (1u << 20) | (Rd << 12) | lsr | (Rd) | (0b1101u << 21)   );
            case 0b0100:
                asr = (1 << 4) | (1 << 6) | (Rs << 8);
                return executeDataProc(     (1u << 20) | (Rd << 12) | asr | (Rd) | (0b1101u << 21)   );
            case 0b0111:
                ror = (1 << 4) | (0b11 << 5) | (Rs << 8);
                return executeDataProc(     (1u << 20) | (Rd << 12) | ror | (Rd) | (0b1101u << 21)   );
            case 0b1001:
                return executeDataProc(     (1u << 25) | (1u << 20) | (Rd << 12) | (Rs << 16) | (0b0011u << 21)   );
            default:
                return executeMultiply(     (1u << 20) | (Rd << 16) | Rs | (Rd << 8)    );
        }
    }
    else{
        Word arm = (1u << 20) | (Rd << 16) | (Rd << 12) | (Rs) | (1u << 20);
        return executeDataProc(arm);

    }

}

bool gba::CPU::executeThumbHiRegisterOperationsBX(Word instruction)
{
    Word Rd = (instruction & 0b111) | ((instruction & 0b10000000) >> 4);
    Word Rs = (instruction & (0b1111u << 3)) >> 3;
    Word opcode = (instruction & (0b11 << 8)) >> 8;
    switch(opcode){
        case 0:
            return executeDataProc(     (Rd << 12) | (Rd << 16) | Rs | (0b0100u << 21)    );
        case 1:
            return executeDataProc(     (Rd << 12) | (Rd << 16) | Rs | (0b1010u << 21) | (1u << 20)    );
        case 2:
            return executeDataProc(     (Rd << 12) | (Rd << 16) | Rs | (0b1101u << 21)    );
        default:
            return executeBranchExchange(   Rs    );
    }
}

bool gba::CPU::executeThumbPCRelativeLoad(Word instruction)
{
    Word imm = instruction & 0xFF;
    Word Rd = (instruction & (0b111u << 8)) >> 8;
    return executeSingleDataTransfer(   (imm) | (Rd << 12) | (R15 << 16) | (1u << 20) | (1u << 23) | (1u << 24)    );
}

bool gba::CPU::executeThumbLoadStoreWithRegisterOff(Word instruction)
{
    Word Rd = instruction & 0b111;
    Word Rb = (instruction & (0b111u << 3)) << 3;
    Word Ro = (instruction & (0b111u << 6)) << 6;
    bool B = instruction & (1u << 10);
    bool L = instruction & (1u << 11);
    return executeSingleDataTransfer(   Ro | (Rb << 16) | (Rd << 12) | (L << 20) | (B << 22) | (1u << 23) | (1u << 24) | (1u << 25)    );
}

bool gba::CPU::executeThumbLoadStoreSignExtendedBHW(Word instruction)
{
    Word Rd = instruction & 0b111;
    Word Rb = (instruction & (0b111 << 3)) >> 3;
    Word Ro = (instruction & (0b111 << 6)) >> 6;
    bool S = instruction & (1u << 10);
    bool H = instruction & (1u << 11);
    bool L = S || H;
    Word sh;
    if(L) sh = (S << 1) | H;
    else sh = 0b01;
    return executeDataTransferSignHDW(  (1u << 24) | (1u << 23) | (L << 20) | (Rd << 12) | (Ro) | (Rb << 16) | (sh << 5)    );
}

bool gba::CPU::executeThumbLoadStoreWithImmOffset(Word instruction)
{
    Word Rd = instruction & 0b111;
    Word Rb = (instruction & (0b111 << 3)) >> 3;
    Word offset = (instruction & (0b11111 << 6)) >> 6;
    bool L = instruction & (1u << 11);
    bool B = instruction & (1u << 12);

    return executeSingleDataTransfer(   offset | (Rb << 16) | (Rd << 12) | (L << 20) | (B << 22) | (1u << 23) | (1u << 24)   );
}

bool gba::CPU::executeThumbLoadStoreHalfWord(Word instruction)
{
    Word Rd = instruction & 0b111;
    Word Rb = (instruction & (0b111u << 3)) >> 3;
    Word offset = (instruction & (0b11111u << 6)) >> 6;
    Word offsetHi = (offset & 0b10000) >> 4;
    bool L = instruction & (1u << 11);

    return executeDataTransferSignHDW(  (1u << 24) | (1u << 23) | (L << 20) | (Rd << 12) | (Rb << 16) | (offset & 0xF) | (offsetHi << 8) | (1u << 5)   );
}

bool gba::CPU::executeThumbSPRelativeLoadStore(Word instruction)
{
    Word imm = instruction & 0xFF;
    Word Rd = (instruction & (0b111u << 8)) >> 8;
    bool L = instruction & (1u << 11);
    executeSingleDataTransfer(   imm | (R13 << 16) | (Rd << 12) | (L << 20) | (1u << 23) | (1u << 24)   );
    return false;
}

bool gba::CPU::executeThumbLoadAddress(Word instruction)
{
    Word imm = instruction & 0xFF;
    Word Rd = (instruction & (0b111u << 8)) >> 8;
    bool SP = instruction & (1u << 11);
    Word source = SP ? R13 : R15;
    // Rotieren nach rechts um 30? Entspricht << 2
    // Datasheet sagt, das ist ein 10 bit Wert
    //                                                                                              rotate
    return executeDataProc(   imm | (Rd << 12) | (source << 16) | (0b0100u << 21) | (1u << 25) | (0xFu << 8)  );
}

bool gba::CPU::executeThumbAddOffsetToSP(Word instruction)
{
    Word imm = instruction & 0b1111111;
    bool S = instruction & (1u << 7);
    Word opcode = S ? 0b0010 : 0b0100;
    return executeDataProc(   imm | (R13 << 12) | (R13 << 16) | (opcode << 21) | (1u << 25) | (0xFu << 8)  );
}

bool gba::CPU::executeThumbPushPopRegisters(Word instruction)
{
    Word Rlist = instruction & 0xFF;
    bool R = instruction & (1u << 8);
    bool L = instruction & (1u << 11);
    bool U = L ? true : false;
    bool P = L ? false : true;
    if(R && L) Rlist |= (1u << R15);
    else if(R && !L) Rlist |= (1u << R14);
    
    return executeBlockDataTransfer(    Rlist | (R13 << 16) | (L << 20) | (1u << 21) | (U << 23) | (P << 24)    );
}

bool gba::CPU::executeThumbMultipleLoadStore(Word instruction)
{
    Word Rlist = instruction & 0xFF;
    Word Rb = (instruction & (0b111u << 8)) >> 8;
    bool L = instruction & (1u << 11);
    return executeBlockDataTransfer(    Rlist | (Rb << 16) | (L << 20) | (1u << 21) | (1u << 23)   );
}

bool gba::CPU::executeThumbConditionalBranch(Word instruction)
{
    Word offset = instruction & 0xFF;
    Word cond = (instruction & (0xFu << 8)) >> 8;

    if(this->checkCondition((Condition)cond)){
        // Bemerkung im Datasheet: Der Offset ist ein 9-bit Wert! B/BL selbst shiftet nochmal um zwei, also insgesamt 11 bit?
        return executeBranchLink(offset << 1);
    }
    else return false;
}

bool gba::CPU::executeThumbSoftwareInterrupt(Word instruction)
{
    return executeSoftwareInterrupt(instruction & 0xFF);
}

bool gba::CPU::executeThumbUnconditionalBranch(Word instruction)
{
    Word offset = instruction & 0b11111111111;
    return executeBranchLink(   (1u << 24) | (offset << 1)    );
}

bool gba::CPU::executeThumbLongBranchWithLink(Word instruction)
{
    Word offset = instruction & 0b11111111111;
    bool H = instruction & (1u << 11);

    if(!H){
        *registerMap[mode()][R14] = *registerMap[mode()][R15] + (offset << 12);
        return false;
    }
    else{
        Word tmp = *registerMap[mode()][R15] - 2;
        *registerMap[mode()][R14] += (offset << 1);
        *registerMap[mode()][R15] = *registerMap[mode()][R14];
        *registerMap[mode()][R14] = tmp | 1;
        return true;
    }
}
