#include "arm7tdmi.h"
#include <bitset>
#include <iostream>
#include <bit>
#include <algorithm>

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
    // Der Inhalt des Registers?
    CpuState nextState = (*registerMap[mode()][Rn] & 1) > 0 ? THUMB : ARM;
    Word jumpTarget = *registerMap[mode()][Rn] & ~(1u);

    *registerMap[mode()][R15] = jumpTarget;
    this->_CPSR.state.T = nextState;
    this->remainingCycles += 3; //2S + 1N
    return true;
}

bool gba::CPU::executeSingleDataSwap(Word instruction)
{
    Word Rm = instruction & 0b1111; // source
    Word Rn = (instruction & (0b1111u << 16)) >> 16; // adresse
    Word address = *registerMap[mode()][Rn];
    Word Rd = (instruction & (0b1111u << 12)) >> 12; // dest
    bool swapByteOnly = instruction & (1u << 22);
    if (swapByteOnly){
        Byte contents = readByte(address);
        writeByte(address, *registerMap[mode()][Rm]);
        *registerMap[mode()][Rd] = contents;
    }
    else{
        Word contents = readWord(address);
        writeWord(address, *registerMap[mode()][Rm]);
        *registerMap[mode()][Rd] = contents;
    }
    remainingCycles += 4; // 1S + 2N + 1I
    return false;
}

bool gba::CPU::executeBranchLink(Word instruction)
{
    bool L = instruction & (1u << 24);
    Word jumpRaw = (instruction & 0x00FFFFFF) << 2;
    int32_t relJump = std::bit_cast<int32_t>(sign_extend_26_32(jumpRaw));

    if(L){
        *registerMap[mode()][R14] = _R15_PC - (state() == ARM ? 4 : 2); // Adresse NACH aktueller (Wegen Pipelining zwei voraus)
    }
    _R15_PC += relJump;
    remainingCycles += 3; //2S + 1N
    return true;
}


// Verhalten mit S gesetzt unklar, mögliche Fehlerquelle
// Verhalten mit R15 als Basis unklar, mögliche Fehlerquelle, sollte aber eigentlich nicht benutzt werden
bool gba::CPU::executeBlockDataTransfer(Word instruction)
{
    Word Rn = (instruction & (0b1111u << 16)) >> 16;
    const bool L = instruction & (1u << 20);
    const bool W = instruction & (1u << 21);
    const bool S = instruction & (1u << 22);
    const bool U = instruction & (1u << 23);
    const bool P = instruction & (1u << 24);
    const int incr = 4;
    bool firstRegister = true;
    int numberOfRegs = std::popcount(instruction & 0xFFFF);
    int downOffset = !U ? (numberOfRegs+1) * 4 : 0; // Wortgröße 4
    downOffset -= !U && !P ? 8 : 0; // Magie, empirisch bestimmt
    
    bool baseInRegisterList = instruction & (1u << Rn);
    bool R15InTransferList = instruction & (1u << 15);
    int actualMode = (S && !L && R15InTransferList) || (!R15InTransferList && S) ? SystemUser : mode();
    bool transferCPSR = L && S && R15InTransferList;
    Word origAddr = *registerMap[actualMode][Rn];
    Word addr = origAddr;
    
    for(int i=0; i < 16; i++){
        bool useRegister = instruction & (1u << i);
        if(useRegister){
            if((unsigned int)i<Rn) firstRegister = false; 
        }
    }

    if(W){ // Aufpassen, überprüfen
        if(!baseInRegisterList || (!L || !firstRegister))
            *registerMap[actualMode][Rn] = origAddr + ( (U ? 1 : -1) * numberOfRegs * incr);
    }

    for(unsigned int i=0; i < 16; i++){
        bool useRegister = instruction & (1u << i);
        if(useRegister){
            if(P) addr += incr;
            
            if(L){ // Aus Speicher laden LDM
                *registerMap[actualMode][i] = readWordUnaligned(addr - downOffset);
            }
            else{ // In Speicher schreiben STM
                if(i==Rn && baseInRegisterList && firstRegister){
                    writeWordUnaligned(addr - downOffset, origAddr);
                }
                else{
                    if(i!=15) writeWordUnaligned(addr - downOffset, *registerMap[actualMode][i]);
                    else writeWordUnaligned(addr - downOffset, *registerMap[actualMode][i] + 4); // Adresse der Anweisung + 12
                }
            }
            
            if(!P) addr += incr;

            if(transferCPSR && i==15){
                if(actualMode != SystemUser){
                    *registerMap[actualMode][CPSR] = *registerMap[actualMode][SPSR];
                } // else?
            }
        }
    }
    bool pcAsBase = false;
    if(Rn==15){
        pcAsBase = true;
    }

    if(L){
        remainingCycles += numberOfRegs + 2; // LDM: n*S + 1N + 1I
    }
    else{
        remainingCycles += (numberOfRegs-1) + 2; //STM: (n-1)*S + 2N
    }
    if((L && (instruction & (1u << 15))) || pcAsBase){ // LDM PC
        return true;
    }
    return false;
}

bool gba::CPU::executeSoftwareInterrupt(Word instruction)
{
    (void)instruction;
    _R14_SVC = *registerMap[mode()][R15] + 4;
    *registerMap[mode()][R15] = 0x08;
    _SPSR_SVC.raw = *registerMap[mode()][CPSR];
    _CPSR.state.mode_bits = Supervisor;
    remainingCycles += 3; // 2S + 1N
    return true;
}

bool gba::CPU::executeCoProcDataOperation(Word instruction)
{
    std::cout << "Coprozessor-Datenoperation aufgerufen!" << instruction << std::endl;
    return false;
}

bool gba::CPU::executeCoProcDataTransfer(Word instruction)
{
    std::cout << "Coprozessor-Datentransfer aufgerufen!" << instruction << std::endl;
    return false;
}

bool gba::CPU::executeCoProcRegTransfer(Word instruction)
{
    std::cout << "Coprozessor-Registertransfer aufgerufen!" << instruction << std::endl;
    return false;
}

bool gba::CPU::executeUndefined(Word instruction) // Falsch implementiert
{
    std::cout << "Undefinierte Instruktion aufgerufen!" << instruction << std::endl;
    _CPSR.state.mode_bits = Undefined;
    return false;
}

namespace gba{
    // (SUB, RSB, ADD, ADC, SBC, RSC, CMP, CMN)
    inline bool setCFlag(Word opcode, Word operand1Value, Word operand2Value, uint64_t result, Word op3){
        switch(opcode){
            case 0b0010:
                return operand1Value >= operand2Value;
            case 0b0011:
                return operand2Value >= operand1Value;
            case 0b0100:
                return (Word)result < operand1Value;
            case 0b0101:
                return result >> 32;
            case 0b0110:
                return (uint64_t)operand1Value >= (uint64_t)operand2Value + (uint64_t)op3;
            case 0b0111:
                return (uint64_t)operand2Value >= (uint64_t)operand1Value + (uint64_t)op3;
            case 0b1010:
                return operand1Value >= operand2Value;
            case 0b1011:
                return (Word)result < operand1Value;
            default:
                return false;
        }
    }
    inline bool setVFlag(Word opcode, Word operand1Value, Word operand2Value, uint64_t result){
        switch(opcode){
            case 0b0010:
                return ((operand1Value ^ operand2Value) & (operand1Value ^ result)) >> 31;
            case 0b0011:
                return ((operand1Value ^ operand2Value) & (operand2Value ^ result)) >> 31;
            case 0b0100:
                return (~(operand1Value ^ operand2Value) & (operand2Value ^ result)) >> 31;
            case 0b0101:
                return (~(operand1Value ^ operand2Value) & (operand2Value ^ (Word)result)) >> 31;
            case 0b0110:
                return ((operand1Value ^ operand2Value) & (operand1Value ^ result)) >> 31;
            case 0b0111:
                return ((operand1Value ^ operand2Value) & (operand2Value ^ result)) >> 31;
            case 0b1010:
                return ((operand1Value ^ operand2Value) & (operand1Value ^ (Word)result)) >> 31;
            case 0b1011:
                return (~(operand1Value ^ operand2Value) & (operand2Value ^ result)) >> 31;
            default:
                return false;
        }
    }
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
    Word operand1Value = *registerMap[mode()][Rn];
    Word operand2 = instruction & 0xFFF;
    Word operand2Value = 0;
    bool logicalCarry = false; // CPSR-C Wert, falls logische Operation
    const bool carryBefore = (std::bit_cast<StatusRegister>(*registerMap[mode()][CPSR])).state.C;
    bool keepOldCarry = false;
    bool registerSpecifiedShift = false;
    Word op3 = 0;

    if(I){ // Immediate
        Word imm = operand2 & 0xFF;
        Word rotate = (operand2 & (0b1111u << 8)) >> 8;
        if(rotate > 0){
            logicalCarry = (imm >> ((2 * rotate) - 1)) & 1u;
            operand2Value = std::rotr(imm, 2 * rotate);
        }
        else{
            logicalCarry = carryBefore;
            operand2Value = imm;
        }
    }
    else{ // Register
        Word op2RegValue = *registerMap[mode()][operand2 & 0xF];
        ShiftType shift = (ShiftType)((operand2 & (0b11 << 5)) >> 5);
        Word amount = 0;
        if(operand2 & (1u << 4)){ // Shift Register
            Word RsValue = *registerMap[mode()][(operand2 & (0xFu << 8)) >> 8];
            if(Rn == R15){// Bei Registershift Amount und R15 als Operand ist PC nochmals 4 Bytes voran, nicht für Rs! (aus Testfällen gelernt)
                operand1Value += 4;
            }
            if((operand2 & 0xF) == R15){ // Op2
                op2RegValue += 4;
            }
            amount = RsValue & 0xFF; //Nur unterstes Byte
            registerSpecifiedShift = true;
        }
        else{ // Shift Immediate
            amount = (operand2 & (0b11111u << 7)) >> 7; // 5 bit unsigned
        }
        if(!registerSpecifiedShift || amount > 0){
            switch(shift){
                case LogicalLeft:
                    logicalCarry = amount <= 32 ? op2RegValue & (1u << (32 - amount)) : false; // Unterstes rausgeshiftetes Bit
                    if(amount==0) logicalCarry = carryBefore; // In diesem Fall bleibt C erhalten
                    if(amount >= 32){ // UB in C++
                        operand2Value = 0;
                    }
                    else operand2Value = op2RegValue << amount;
                    break;
                case LogicalRight:
                    if(amount == 0) amount = 32;
                    if(amount >= 32){ // UB in C++
                        logicalCarry = 0;
                        if(amount == 32) logicalCarry = op2RegValue & (1u << 31);
                        operand2Value = 0;
                    }
                    else{
                        logicalCarry = op2RegValue & (1u << (amount - 1));
                        operand2Value = op2RegValue >> amount;
                    }
                    break;
                case ArithmeticRight:{
                    if(amount == 0) amount = 32;
                    if(amount < 32){
                        logicalCarry = op2RegValue & (1u << (amount - 1));
                    }
                    else logicalCarry = op2RegValue & (1u << 31);
                    operand2Value = op2RegValue;
                    // Wie langsam ist das?
                    for(unsigned int i = 0; i < amount; i++){
                        Word lastbit = operand2Value & (1u << 31);
                        operand2Value >>= 1;
                        operand2Value = operand2Value | lastbit;
                    }
                    break;}
                case RotateRight:
                    if(amount > 0){ // RR
                        logicalCarry = op2RegValue & (1u << (amount - 1));
                        operand2Value = std::rotr(op2RegValue, amount);
                    }
                    else{//RRX
                        logicalCarry = op2RegValue & 1u; // bit 0
                        operand2Value = (op2RegValue >> 1) | (carryBefore << 31); // Carry bit wird links reingeshiftet
                    }
                    break;
            }
        }
        else{
            operand2Value = op2RegValue;
            keepOldCarry = true;
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
            result = (uint64_t)operand1Value + (uint64_t)operand2Value + (uint64_t)carryBefore;
            break;
        case 0b0110: // SBC
            op3 =(Word)carryBefore ^ 1;
            result = operand1Value - operand2Value - op3;
            break;
        case 0b0111: // RSC
            op3 =(Word)carryBefore ^ 1;
            result = operand2Value - operand1Value - op3;
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
            result = ~operand2Value;
            break;
    }
    Word result32 = (Word)result;

    // Bei Testoperationen nicht Ergebnis schreiben
    if(opcode < 0b1000 || opcode > 0b1011){
        *registerMap[mode()][Rd] = result32;
    }

    if(S){
        if(Rd==15 && mode() != SystemUser){
            *registerMap[mode()][CPSR] = *registerMap[mode()][SPSR];
        }
        else{
            //(AND, EOR, TST, TEQ, ORR, MOV, BIC, MVN):
            // V bleibt
            // C - Carry out aus dem Barrel Shifter, bzw. bleibt erhalten bei LSL #0
            // Z - gesetzt, wenn Ergebnis 0
            // N - Bit 31
            if(opcode <= 1 || opcode >= 0b1100 || opcode == 0b1000 || opcode == 0b1001){
                StatusRegister cpsr = std::bit_cast<StatusRegister>(*registerMap[mode()][CPSR]);
                cpsr.state.C = keepOldCarry ? carryBefore : logicalCarry;
                cpsr.state.Z = result32 == 0;
                cpsr.state.N = (result32 & (1u << 31)) > 0;
                *registerMap[mode()][CPSR] = cpsr.raw;
            }
    
            // (SUB, RSB, ADD, ADC, SBC, RSC, CMP, CMN):
            //  If the S bit is set (and Rd is not R15) the V flag in the CPSR will be set if an overflow occurs into bit 31 of the result
            // C - Carry out bit aus bit 31 der ALU
            // Z - gesetzt, wenn Ergebnis 0
            // N - Bit 31
            else{
                StatusRegister cpsr = std::bit_cast<StatusRegister>(*registerMap[mode()][CPSR]);
                cpsr.state.V = setVFlag(opcode, operand1Value, operand2Value, result);
                cpsr.state.C = setCFlag(opcode, operand1Value, operand2Value, result, op3);
                cpsr.state.Z = result32 == 0;
                cpsr.state.N = (result32 & (1ull << 31)) > 0;
                *registerMap[mode()][CPSR] = cpsr.raw;
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
    if(Rd == R15 && (opcode < 0b1000 || opcode > 0b1011)){ // PC geschrieben
        return true;
    }
    // std::cout << operand1Value << std::endl;
    // std::cout << operand2Value << std::endl;
    else return false;
}

bool gba::CPU::executePSRTransfer(Word instruction)
{
    Word form_MSRBitsOnly = 0b0000'0001'0010'1000'1111'0000'0000'0000; // (Register oder imm nach PSR nur flag bits)
    Word mask_MSRBitsOnly = 0b0000'1101'1011'1111'1111'0000'0000'0000;

    Word form_MSR =         0b0000'0001'0010'1001'1111'0000'0000'0000; // (Register nach PSR)
    Word mask_MSR =         0b0000'1111'1011'1111'1111'1111'1111'0000;

    auto masked = instruction & mask_MSRBitsOnly;
    bool fits = masked == form_MSRBitsOnly;

    auto masked2 = instruction & mask_MSR;
    bool fits2 = masked2 == form_MSR;

    if(fits){ // MSR Bits only
        bool destSPSRMode = instruction & (1u << 22);
        bool I = instruction & (1u << 25);
        Word val;
        if(I){
            val = instruction & 0xFF;
            // Genau wie in DataProc?
            Word rotate = (instruction & (0xFu << 8)) >> 8;
            val = std::rotr(val, 2 * rotate);
            val &= 0xFu << 28; // obere Bits maskieren
        }
        else{ // Registerinhalt
            Word sourceReg = instruction & 0xF;
            val = *registerMap[mode()][sourceReg] & (0xFu << 28);
        }

        if(destSPSRMode){
            *registerMap[mode()][SPSR] =  (*registerMap[mode()][SPSR] & ~(0xFu << 28)) | val;
        }
        else{
            *registerMap[mode()][CPSR] = (*registerMap[mode()][CPSR] & ~(0xFu << 28)) | val;
        }
    }
    else if(fits2){ // MSR (Unklar: Im Usermode() kann MSR nicht die unteren Kontrollbits von CPSR ändern, läuft der GBA in User oder System? Ist das relevant? SPSR existiert auch nicht im Usermode().)
        bool destSPSRMode = instruction & (1u << 22);
        Word sourceReg = instruction & 0xFu;
        Word val = *registerMap[mode()][sourceReg];
        if(destSPSRMode){
            *registerMap[mode()][SPSR] = val;
        }
        else{
            *registerMap[mode()][CPSR] = val;
        }
    }
    else{ //MRS (PSR nach Register)
        bool sourceSPSRMode = instruction & (1u << 22);
        Word destReg = (instruction & (0xFu << 12)) >> 12;

        Word val;
        if(sourceSPSRMode){
            val = *registerMap[mode()][SPSR];
        }
        else{
            val = *registerMap[mode()][CPSR];
        }
        *registerMap[mode()][destReg] = val;
    }

    remainingCycles += 1; // 1S
    return false;
}

bool gba::CPU::executeSingleDataTransfer(Word instruction)
{
    bool I = instruction & (1u << 25);
    bool P = instruction & (1u << 24);
    bool U = instruction & (1u << 23);
    bool B = instruction & (1u << 22);
    bool W = instruction & (1u << 21);
    bool L = instruction & (1u << 20);
    bool isJump = false;
    Word baseReg = (instruction & (0xFu << 16)) >> 16;
    Word sourceDestReg = (instruction & (0xFu << 12)) >> 12;
    // "In the case of post-indexed addressing, the write back bit is
    // redundant and must be set to zero, since the old base value can be retained by setting
    // the offset to zero. Therefore post-indexed data transfers always write back the modified base"
    // https://iitd-plos.github.io/col718/ref/arm-instructionset.pdf S. 4-27
    if(!P) W = true;
    if(baseReg == R15 && W){
        isJump = true;
    }
    // else if(sourceDestReg == R15) *registerMap[mode()][R15] += 4;
    Word modifiedBase = *registerMap[mode()][baseReg];// + (baseReg == R15 && W ? 4 : 0);
    const bool carryBefore = (std::bit_cast<StatusRegister>(*registerMap[mode()][CPSR])).state.C;

    Word offset;


    auto modifyOffset = [&](){
        if(U){
            modifiedBase += offset;
        }
        else{
            modifiedBase -= offset;
        }
    };

    if(!I){ // 12 bit immediate
        offset = instruction & 0xFFF;
    }
    else{ // register mit Shift
        Word reg = instruction & 0xF;
        Word shift = (instruction & (0xFFu << 4)) >> 4;
        ShiftType shiftType = (ShiftType) ((shift & 0b110) >> 1);
        Word amount = (shift & 0b11111000) >> 3;
        Word regVal = *registerMap[mode()][reg];
        switch(shiftType){
            case LogicalLeft:
                if(amount >= 32){ // UB in C++
                    offset = 0;
                }
                else offset = regVal << amount;
                break;
            case LogicalRight:
                if(amount == 0) amount = 32;
                if(amount >= 32){ // UB in C++
                    offset = 0;
                }
                else{
                    offset = regVal >> amount;
                }
                break;
            case ArithmeticRight:{
                if(amount == 0) amount = 32;
                offset = regVal;
                for(unsigned int i = 0; i < amount; i++){
                    Word lastbit = offset & (1u << 31);
                    offset >>= 1;
                    offset = offset | lastbit;
                }
                break;}
            case RotateRight:
                if(amount > 0){ // RR
                    offset = std::rotr(regVal, amount);
                }
                else{//RRX
                    offset = (regVal >> 1) | (carryBefore << 31); // Carry bit wird links reingeshiftet
                }
                break;
        }
    }

    if(P){ // Pre-Inkrement
        modifyOffset();
    }

    // Transfer... ANNAHME: GBA ist immer little Endian
    if(L){ // LDR
        Word val;
        if(B){ // Byte
            val = readByte(modifiedBase);
        }
        else{ // Wort
            if(modifiedBase % 4 == 0){ // Wort alignt
                val = readWord(modifiedBase);
            }
            else if((modifiedBase & 0b11) == 0b11){ // Datasheet S. 4-28      (Verhalten verifizieren!!!!)
                // val = readWordUnaligned(modifiedBase);
                Word w = readWordUnaligned(modifiedBase);
                val = (w << 8) | (w >> 24);
            }
            else if((modifiedBase & 0b11) == 0b10){
                Word w = readWordUnaligned(modifiedBase);
                val = (w << 16) | (w >> 16);
            }
            else{
                // modifiedBase &= ~(0b1); // Halbwort align.
                Word upper = readWordUnaligned(modifiedBase); // Ende erstes Wort
                val = (upper >> 8) | (upper << 24);
            }
        }
        *registerMap[mode()][sourceDestReg] = val;
    }
    else{ // STR
        if(B){
            writeByte(modifiedBase, *registerMap[mode()][sourceDestReg] + (sourceDestReg == R15 ? 4 : 0));
        }
        else{
            // modifiedBase &= ~(0b11); // Wort Alignment (Immer)
            writeWordUnaligned(modifiedBase, *registerMap[mode()][sourceDestReg] + (sourceDestReg == R15 ? 4 : 0));
        }
    }

    if(!P){ // Post-Inkrement
        if(modifiedBase != *registerMap[mode()][baseReg])
            modifiedBase = *registerMap[mode()][baseReg];
        else modifyOffset();
    }

    if(W){ // Write back
        if(baseReg == sourceDestReg && P && L) modifiedBase = *registerMap[mode()][sourceDestReg];
        *registerMap[mode()][baseReg] = modifiedBase + (baseReg == R15 ? 4 : 0);
    }

    // Sehr seltsamer Spezialfall, empirisch bestimmt
    if(W && sourceDestReg == R15 && baseReg == R15 && 
        L){
        *registerMap[mode()][R15] -= 4;
    }

    if(L){ // LDR
        if(sourceDestReg == R15){ // LDR PC
            isJump = true;
            remainingCycles += 5; // 2S + 2N + 1I
        }
        else{
            remainingCycles += 3; // 1S + 1N + 1I
        }
    }
    else{ // STR
        remainingCycles += 2; // 2N
    }

    return isJump;
}

bool gba::CPU::executeDataTransferSignHDW(Word instruction)
{
    bool S = instruction & (1u << 6); // Sign
    bool H = instruction & (1u << 5); // Halbwort
    Word Rd = (instruction & (0xFu << 12)) >> 12; // Source / Dest
    Word Rn = (instruction & (0xFu << 16)) >> 16; // Base
    
    bool L = instruction & (1u << 20);
    bool W = instruction & (1u << 21);
    bool U = instruction & (1u << 23);
    bool P = instruction & (1u << 24);
    
    bool immediate = (instruction & (1u << 22));
    Word offsetValue;
    bool isJump = L && Rd == R15;

    
    Word modifiedBase = *registerMap[mode()][Rn];
    if(immediate){
        offsetValue = (instruction & 0xF) | ((instruction & (0xFu << 8)) >> 4);
    }
    else{
        Word Rm = instruction & 0xF; // Offset Register
        offsetValue = *registerMap[mode()][Rm];
    }

    if(!P) W = true; // Wie in der anderen

    auto modifyOffset = [&](){
        if(U){
            modifiedBase += offsetValue;
        }
        else{
            modifiedBase -= offsetValue;
        }
    };

    if(P) modifyOffset();

    // Operation hier:
    // S==0 und H==0 wird nicht beachtet, weil es sich dann um Single Data Swap handelt
    if(S){ // Sign extend Halbwort oder Byte
        Word val;
        if(H){
            val = readHalfWord(modifiedBase);
            Word signBit = 15;
            if (modifiedBase & 1) {
                val = (val >> 8) | (val << 24);
                signBit = 7;
            }
            val &= 0xFFFFu;
            Word mask = 1u << signBit;
            val = (val ^ mask) - mask;
        }
        else{
            val = readByte(modifiedBase);
            val &= 0xFFu;
            Word mask = 1u << 7;
            val = (val ^ mask) - mask;
        }
        // ist immer LDR
        *registerMap[mode()][Rd] = val;
    }
    else{ // Unsigned Halbwort
        if(L){
            Word val = readHalfWord(modifiedBase);
            // Siehe Nanoboyadvance
            if (modifiedBase & 1) {
              val = (val >> 8) | (val << 24);
            }
            *registerMap[mode()][Rd] = val;
        }
        else{
            Word R15Offset = Rd == R15 ? 4 : 0;
            writeHalfWord(modifiedBase, *registerMap[mode()][Rd] + R15Offset);
        }
    }

    
    if(!P){
        if(modifiedBase != *registerMap[mode()][Rn])
            modifiedBase = *registerMap[mode()][Rn];
        else modifyOffset();
    }

    if(W){
        if(Rn == R15){
            modifiedBase += 4;
            isJump = true;
        }
        if(!(L && P) || Rd != Rn)
            *registerMap[mode()][Rn] = modifiedBase;
    }

    // Sehr seltsamer Spezialfall, empirisch bestimmt
    if(W && Rd == R15 && Rn == R15 && 
        L && !P){
        *registerMap[mode()][R15] -= 4;
    }
    

    if(L){
        if(Rd == 15){ // LDR PC
            remainingCycles += 5; // 2S + 2N + 1I
        }
        else{
            remainingCycles += 3; // 1S + 1N + 1I
        }
    }
    else{ // STRH
        remainingCycles += 2; // 2N
    }

    return isJump;
}

bool gba::CPU::executeMultiply(Word instruction)
{
    bool A = instruction & (1u << 21);
    bool S = instruction & (1u << 20);
    Word Rm = instruction & 0xF;
    Word Rs = (instruction & (0xFu << 8)) >> 8;
    Word Rn = (instruction & (0xFu << 12)) >> 12;
    Word Rd = (instruction & (0xFu << 16)) >> 16;

    Word RsOp = *registerMap[mode()][Rs];

    Word result = (*registerMap[mode()][Rm]) * (*registerMap[mode()][Rs]); // Rd := Rm * Rs
    if(A){ // Rd := Rm * Rs + Rn
        result += *registerMap[mode()][Rn];
    }

    *registerMap[mode()][Rd] = result;

    if(S){
        StatusRegister cpsr = std::bit_cast<StatusRegister>(*registerMap[mode()][CPSR]);
        cpsr.state.C = 0; // "Meaningless value"
        cpsr.state.Z = result == 0;
        cpsr.state.N = (result & (1u << 31)) > 0;
        *registerMap[mode()][CPSR] = cpsr.raw;
    }

    Word m; // Spezifiziert durch Rs
    constexpr Word b_31_8 = (0xFFFFFF << 8);
    constexpr Word b_31_16 = (0xFFFF << 16);
    constexpr Word b_31_24 = (0xFF << 24);

    if(((RsOp & b_31_8) == 0 ) || ((RsOp & b_31_8) == b_31_8)){
        m = 1;
    }
    else if(((RsOp & b_31_16) == 0 ) || ((RsOp & b_31_16) == b_31_16)){
        m = 2;
    }
    else if(((RsOp & b_31_24) == 0 ) || ((RsOp & b_31_24) == b_31_24)){
        m = 3;
    }
    else m = 4;

    remainingCycles += 1 + m; // 1S + mI
    if(A) remainingCycles += 1; // 1S + (m+1)I

    return false;
}

bool gba::CPU::executeMultiplyLong(Word instruction)
{
    Word Rm = instruction & 0xF;
    Word Rs = (instruction & (0xFu << 8)) >> 8;
    Word RsOp = *registerMap[mode()][Rs];
    Word RdLo = (instruction & (0xFu << 12)) >> 12;
    Word RdHi = (instruction & (0xFu << 16)) >> 16;

    bool S = instruction & (1u << 20);
    bool A = instruction & (1u << 21);
    bool U = instruction & (1u << 22);

    uint64_t result;
    if(A){
        if(U){
            uint64_t rd = ((uint64_t) *registerMap[mode()][RdLo]) | (((uint64_t) *registerMap[mode()][RdLo]) << 32);
            result = ((uint64_t) *registerMap[mode()][Rm]) * ((uint64_t) *registerMap[mode()][Rs]) + rd;
        }
        else{
            uint64_t wideRm = (uint64_t)*registerMap[mode()][Rm];
            uint64_t wideRs = (uint64_t)*registerMap[mode()][Rs];
            uint64_t rd = ((uint64_t) *registerMap[mode()][RdLo]) | (((uint64_t) *registerMap[mode()][RdLo]) << 32);
            int64_t sresult = (std::bit_cast<int64_t>(wideRm)) * (std::bit_cast<int64_t>(wideRs)) + std::bit_cast<int64_t>(rd);
            result = std::bit_cast<uint64_t>(sresult);
        }
    }
    else{
        if(U){
            result = ((uint64_t) *registerMap[mode()][Rm]) * ((uint64_t) *registerMap[mode()][Rs]);
        }
        else{
            uint64_t wideRm = (uint64_t)*registerMap[mode()][Rm];
            uint64_t wideRs = (uint64_t)*registerMap[mode()][Rs];
            int64_t sresult = (std::bit_cast<int64_t>(wideRm)) * (std::bit_cast<int64_t>(wideRs));
            result = std::bit_cast<uint64_t>(sresult);
        }
    }

    Word resHi = (result & (0xFFFFFFFFull << 32)) >> 32;
    Word resLo = result & 0xFFFFFFFFull;

    *registerMap[mode()][RdLo] = resLo;
    *registerMap[mode()][RdHi] = resHi;



    if(S){
        StatusRegister cpsr = std::bit_cast<StatusRegister>(*registerMap[mode()][CPSR]);
        cpsr.state.C = 0; // "Meaningless value"
        cpsr.state.V = 0; // genau wie C
        cpsr.state.Z = result == 0;
        cpsr.state.N = (result & (1ull << 63)) > 0;
        *registerMap[mode()][CPSR] = cpsr.raw;
    }


    Word m; // Spezifiziert durch Rs
    constexpr Word b_31_8 = (0xFFFFFF << 8);
    constexpr Word b_31_16 = (0xFFFF << 16);
    constexpr Word b_31_24 = (0xFF << 24);

    if(U){
        if(((RsOp & b_31_8) == 0 ) || ((RsOp & b_31_8) == b_31_8)){
            m = 1;
        }
        else if(((RsOp & b_31_16) == 0 ) || ((RsOp & b_31_16) == b_31_16)){
            m = 2;
        }
        else if(((RsOp & b_31_24) == 0 ) || ((RsOp & b_31_24) == b_31_24)){
            m = 3;
        }
        else m = 4;
    }
    else{
        if((RsOp & b_31_8) == 0 ){
            m = 1;
        }
        else if((RsOp & b_31_16) == 0){
            m = 2;
        }
        else if((RsOp & b_31_24) == 0){
            m = 3;
        }
        else m = 4;
    }


    remainingCycles += 1 + m + 1; // 1S + (m+1)I
    if(A) remainingCycles += 1; // 1S + (m+2)I

    return false;
}
