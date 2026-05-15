#include "arm7tdmi.h"
#include <iostream>
#include <bit>

// Source - https://stackoverflow.com/a/42536138
// Posted by user555045, modified by community. See post 'Timeline' for change history
// Retrieved 2026-04-03, License - CC BY-SA 3.0

uint32_t sign_extend_26_32(uint32_t x) {
    const int bits = 26;
    uint32_t m = 1u << (bits - 1);
    return (x ^ m) - m;
}


bool gba::CPU::executeBranchExchange(Word instruction)
{
    Word Rn = instruction & 0b1111;
    CpuState nextState = (Rn & 1) > 0 ? THUMB : ARM;
    Word jumpTarget = *registerMap[mode][Rn];

    *registerMap[mode][R15] = jumpTarget;
    this->state = nextState;
    this->remainingCycles += 3; //2S + 1N
    return true;
}

bool gba::CPU::executeSingleDataSwap(Word instruction)
{
    Word Rm = instruction & 0b1111; // source
    Word Rn = (instruction & (0b1111u << 16)) >> 16; // adresse
    Word address = *registerMap[mode][Rn];
    Word Rd = (instruction & (0b1111u << 12)) >> 12; // dest
    bool swapByteOnly = instruction & (1u << 22);
    if (swapByteOnly){
        Byte contents = readByte(address);
        writeByte(address, *registerMap[mode][Rm]);
        *registerMap[mode][Rd] = contents;
    }
    else{
        Word contents = readWord(address);
        writeWord(address, *registerMap[mode][Rm]);
        *registerMap[mode][Rd] = contents;
    }
    remainingCycles += 4; // 1S + 2N + 1I
    return false;
}

bool gba::CPU::executeBranchLink(Word instruction)
{
    bool L = instruction & (1u << 24);
    Word jumpRaw = (instruction & 0x00FFFFFF) << 2;
    int32_t relJump = sign_extend_26_32(jumpRaw);

    if(L){
        *registerMap[mode][R14] = _R15_PC - 1; // Adresse NACH aktueller (Wegen Pipelining zwei voraus)
    }
    _R15_PC += relJump;
    remainingCycles += 3; //2S + 1N
    return true;
}

bool gba::CPU::executeBlockDataTransfer(Word instruction)
{
    Word Rn = (instruction & (0b1111u << 16)) >> 16;
    const bool L = instruction & (1u << 20);
    const bool W = instruction & (1u << 21);
    const bool S = instruction & (1u << 22); // Ignoriere ich hier aus Einfachheit, nicht implementiert
    const bool U = instruction & (1u << 23);
    const bool P = instruction & (1u << 24);
    const int incr = U ? +4 : -4;
    Word addr = *registerMap[mode][Rn] & ~0b11u; // Alignment
    bool firstRegister = true;
    int n = 0;
    for(int i=0; i < 16; i++){
        bool useRegister = instruction & (1u << i);
        if(useRegister){
            n++;
            if(P) addr += incr;
            
            if(L){ // Aus Speicher laden LDM
                *registerMap[mode][i] = readWord(addr);
            }
            else{ // In Speicher schreiben STM
                if(i!=15)
                    writeWord(addr, *registerMap[mode][i]);
                else writeWord(addr, *registerMap[mode][i] + 10); // Adresse der Anweisung + 12
            }
            
            if(!P) addr += incr;
            if(!firstRegister && W && !L){
                *registerMap[mode][Rn] = addr;
            }
            firstRegister = false;
        }
    }
    if(L){
        remainingCycles += n + 2; // LDM: n*S + 1N + 1I
    }
    else{
        remainingCycles += (n-1) + 2; //STM: (n-1)*S + 2N
    }
    if(!L && (instruction & (1u << 15))){ // LDM PC
        return true;
    }
    return false;
}

bool gba::CPU::executeSoftwareInterrupt(Word instruction)
{
    _R14_SVC = *registerMap[mode][R15] + 4;
    *registerMap[mode][R15] = 0x08;
    _SPSR_SVC.raw = *registerMap[mode][CPSR];
    this->mode = Supervisor;
    remainingCycles += 3; // 2S + 1N
    return true;
}

bool gba::CPU::executeCoProcDataOperation(Word instruction)
{
    std::cout << "Coprozessor-Datenoperation aufgerufen!" << std::endl;
    return false;
}

bool gba::CPU::executeCoProcDataTransfer(Word instruction)
{
    std::cout << "Coprozessor-Datentransfer aufgerufen!" << std::endl;
    return false;
}

bool gba::CPU::executeCoProcRegTransfer(Word instruction)
{
    std::cout << "Coprozessor-Registertransfer aufgerufen!" << std::endl;
    return false;
}

bool gba::CPU::executeUndefined(Word instruction) // Falsch implementiert
{
    std::cout << "Undefinierte Instruktion aufgerufen!" << std::endl;
    this->mode = Undefined;
    return false;
}

enum ShiftType{
    LogicalLeft = 0b00,
    LogicalRight = 0b01,
    ArithmeticRight = 0b10,
    RotateRight = 0b11,
};

// Möglicher Fallstrick: 4.5.5 Using R15 as an operand
// If R15 (the PC) is used as an operand in a data processing instruction the register is
// used directly.
// The PC value will be the address of the instruction, plus 8 or 12 bytes due to instruction
// prefetching. If the shift amount is specified in the instruction, the PC will be 8 bytes
// ahead. If a register is used to specify the shift amount the PC will be 12 bytes ahead.
bool gba::CPU::executeDataProc(Word instruction)
{
    bool I = instruction & (1u << 25);
    bool S = instruction & (1u << 20);
    Word opcode = (instruction & (0b1111 << 21)) >> 21;
    Word Rn = (instruction & (0b1111 << 16)) >> 16;
    Word Rd = (instruction & (0b1111 << 12)) >> 12;
    Word operand1Value = *registerMap[mode][Rn];
    Word operand2 = instruction & 0xFFF;
    Word operand2Value = 0;
    bool logicalCarry = false; // CPSR-C Wert, falls logische Operation
    const bool carryBefore = ((StatusRegister)(*registerMap[mode][CPSR])).C;
    bool registerSpecifiedShift = false;

    if(I){ // Immediate
        Word imm = operand2 & 0xFF;
        Word rotate = (operand2 & (0b1111u << 8)) >> 8;
        operand2Value = std::rotr(imm, 2 * rotate);
    }
    else{ // Register
        Word op2RegValue = *registerMap[mode][operand2 & 0b1111];
        ShiftType shift = (ShiftType)((operand2 & (0b11 << 5)) >> 5);
        Word amount = 0;
        if(operand2 & 1){ // Shift Register
            amount = (*registerMap[mode][(operand2 & (0b1111 << 8)) >> 8]) & 0xFF; //Nur unterstes Byte
            registerSpecifiedShift = true;
        }
        else{ // Shift Immediate
            amount = operand2 & (0b11111u << 7); // 5 bit unsigned
        }
        switch(shift){
            case LogicalLeft:
                logicalCarry = amount <= 32 ? op2RegValue & (1u << (32 - amount)) : false; // Unterstes rausgeshiftetes Bit
                if(amount==0) logicalCarry = carryBefore; // In diesem Fall bleibt C erhalten
                operand2Value = op2RegValue << amount;
                break;
            case LogicalRight:
                if(amount == 0) amount = 32;
                logicalCarry = op2RegValue & (1u << (amount - 1));
                operand2Value = op2RegValue >> amount;
                break;
            case ArithmeticRight:{
                if(amount == 0) amount = 32;
                logicalCarry = op2RegValue & (1u << (amount - 1));
                // In der Hoffnung, dass das funktioniert
                int32_t temp = std::bit_cast<int32_t>(op2RegValue);
                temp = temp >> amount;
                operand2Value = std::bit_cast<Word>(temp);
                break;}
            case RotateRight:
                if(amount > 0){ // RR
                    logicalCarry = op2RegValue & (1u << (amount - 1));
                    operand2Value = std::rotr(op2RegValue, amount);
                }
                else{//RRX
                    logicalCarry = op2RegValue & 1u; // bit 0
                    operand2Value = (op2RegValue >> 1) & (carryBefore << 31); // Carry bit wird links reingeshiftet
                }
                break;
        }
    }

    uint64_t result;

    switch(opcode){
        case 0b0000: // AND
            result = operand1Value & operand2Value;
            break;
        case 0b0001: // EOR
            result = operand1Value ^ operand2Value;
            break;
        case 0b0010: // SUB
            result = operand1Value - operand2Value;
            break;
        case 0b0011: // RSB
            result = operand2Value - operand1Value;
            break;
        case 0b0100: // ADD
            result = operand1Value + operand2Value;
            break;
        case 0b0101: // ADC
            result = operand1Value + operand2Value + (Word)carryBefore;
            break;
        case 0b0110: // SBC
            result = operand1Value - operand2Value + (Word)carryBefore - 1;
            break;
        case 0b0111: // RSC
            result = operand2Value - operand1Value + (Word)carryBefore - 1;
            break;
        case 0b1000: // TST
            result = operand1Value & operand2Value; // result wird nicht geschrieben
            break;
        case 0b1001: // TEQ
            result = operand1Value ^ operand2Value; // result wird nicht geschrieben
            break;
        case 0b1010: // CMP
            result = operand1Value - operand2Value; // result wird nicht geschrieben
            break;
        case 0b1011: // CMN
            result = operand1Value + operand2Value; // result wird nicht geschrieben
            break;
        case 0b1100: // ORR
            result = operand1Value | operand2Value;
            break;
        case 0b1101: // MOV
            result = operand2Value;
            break;
        case 0b1110: // BIC
            result = operand1Value & ~operand2Value;
            break;
        default:     // MVN
            result = ~operand2;
            break;
    }
    Word result32 = (Word)result;
    // carry out xor carry in
    bool overflow = ((result & (1ull << 32)) > 0) != ((result & (1ull << 31)) > 0);

    // Bei Testoperationen nicht Ergebnis schreiben
    if(opcode < 0b1000 || opcode > 0b1011){
        *registerMap[mode][Rd] = result32;
    }

    if(S){
        if(Rd==15){
            if(mode==SystemUser){
                std::cout << "Illegale Operation im Usermodus" << std::endl;
                throw;
            }
            *registerMap[mode][CPSR] = *registerMap[mode][CPSR_OTHER];
        }
        else{
            //(AND, EOR, TST, TEQ, ORR, MOV, BIC, MVN):
            // V bleibt
            // C - Carry out aus dem Barrel Shifter, bzw. bleibt erhalten bei LSL #0
            // Z - gesetzt, wenn Ergebnis 0
            // N - Bit 31
            if(opcode <= 1 || opcode >= 0b1100 || opcode == 0b1000 || opcode == 0b1001){
                StatusRegister cpsr = (StatusRegister)(*registerMap[mode][CPSR]);
                cpsr.C = logicalCarry;
                cpsr.Z = result32 == 0;
                cpsr.N = (result32 & (1u << 31)) > 0;
                *registerMap[mode][CPSR] = cpsr.raw;
            }
    
            // (SUB, RSB, ADD, ADC, SBC, RSC, CMP, CMN):
            //  If the S bit is set (and Rd is not R15) the V flag in the CPSR will be set if an overflow occurs into bit 31 of the result
            // C - Carry out bit aus bit 31 der ALU
            // Z - gesetzt, wenn Ergebnis 0
            // N - Bit 31
            else{
                StatusRegister cpsr = (StatusRegister)(*registerMap[mode][CPSR]);
                cpsr.V = Rd!=R15 ? overflow : false; // Korrekt?
                cpsr.C = (result & (1ull << 32)) > 0;
                cpsr.Z = result32 == 0;
                cpsr.N = (result32 & (1ull << 31)) > 0;
                *registerMap[mode][CPSR] = cpsr.raw;
            }
        }
    }
    // Normal Data Processing 1S
    // Data Processing with register specified shift 1S + 1I
    // Data Processing with PC written 2S + 1N
    // Data Processing with register specified shift and PC written 2S + 1N + 1I
    remainingCycles += 1;
    if(registerSpecifiedShift) remainingCycles += 1;
    if(Rd == R15) remainingCycles += 2;
    return false;
}

bool gba::CPU::executePSRTransfer(Word instruction)
{
    Word form_MSRBitsOnly = 0b0000'0001'0010'1000'1111'0000'0000'0000;
    Word mask_MSRBitsOnly = 0b0000'1101'1011'1111'1111'0000'0000'0000;

    Word form_MSR =         0b0000'0001'0010'1001'1111'0000'0000'0000;
    Word mask_MSR =         0b0000'1111'1011'1111'1111'1111'1111'0000;

    auto masked = instruction & mask_MSRBitsOnly;
    bool fits = masked == form_MSRBitsOnly;

    auto masked2 = instruction & mask_MSR;
    bool fits2 = masked2 == form_MSR;

    if(fits){ // MSR Bits only

    }
    else if(fits2){ // MSR

    }
    else{ //MRS

    }

    remainingCycles += 1; // 1S
    return false;
}
