#include "6502.h"
#include <limits>
#include <format>
#include <iostream>
#include <algorithm>
#include <iomanip>
#include <string>
#include <cstring>

void Cpu::RESET()
{
    uint8_t low = read((uint8_t*)(uintptr_t)0xFFFC);
    uint8_t high = read((uint8_t*)(uintptr_t)0xFFFD);
    uint16_t addr = (high << 8) | low;
    PC = addr;
    SP -= 3;
    // P = 0b00100000;
    setStatus(STATUS_INTERRUPT_DISABLE, true);
    mapper->write((uint8_t*)0x4015, 0x00);
}

void Cpu::setStatus(Statusbit s, bool v)
{
    if(v){
        P = P | ((uint8_t)1u << s);
    }
    else{
        P = P & ~((uint8_t)1u << s);
    }
}

bool Cpu::getStatus(Statusbit s)
{
    return (P & (1u << s)) >> s;
}

bool willCrossPage(uintptr_t a, uintptr_t b){
    int lowerA = a & 0xff;
    int lowerB = b & 0xff;
    return lowerA + lowerB > static_cast<int>(std::numeric_limits<uint8_t>::max());
}

bool willCrossPage(uintptr_t a, int8_t b){
    uint8_t lowerA = a & 0xff;
    int diff = lowerA + b;
    return (b > std::numeric_limits<uint8_t>::max() - lowerA) || (diff < 0);
}

std::string hex(uintptr_t input){
    std::string str = std::format("{:x}", input);
    std::transform(str.begin(), str.end(), str.begin(), ::toupper);
    return str;
}

std::string hexNorm(std::string s){
    while(s.size() < 4)
        s = "0"+s;
    return s;
}

std::pair<std::string, std::vector<int>> Cpu::getPrev10Instructions()
{
    std::vector<uint16_t> pcList;
    int it = circularIndex;
    for(int i = 0; i < 10; i++){

        int instruction = circular[it];

        if(instruction >=0)
            pcList.push_back(instruction);


        if(it>=9){
            it = 0;
        }
        else{
            it++;
        }
    }

    std::string res = "";
    std::vector<int> length;
    for(const auto &pc : pcList){
        if(pc<0) continue;
        uint8_t opcode = read((uint8_t*)(uintptr_t)pc);
        uint8_t op1 = read((uint8_t*)(uintptr_t)pc+1);
        uint8_t op2 = read((uint8_t*)(uintptr_t)pc+2);
        const opcode_info &info = opcodes[opcode];

        const std::pair<std::string, int> &address = formatInstruction(info.mode, op1, op2, pc);
        res+="$"+hexNorm(hex(pc))+":\t"+info.name+"\t" + address.first+"\n";
        length.push_back(address.second);
    }
    return {res, length};
}

std::pair<std::string, std::vector<int>> Cpu::getNextNInstructions(int n)
{
    uint16_t pc = PC;
    std::string res = "";
    std::vector<int> length;

    for(int i = 0; i < n; i++){
        uint8_t opcode = read((uint8_t*)(uintptr_t)pc);
        uint8_t op1 = read((uint8_t*)(uintptr_t)pc+1);
        uint8_t op2 = read((uint8_t*)(uintptr_t)pc+2);
        const opcode_info &info = opcodes[opcode];

        const std::pair<std::string, int> &address = formatInstruction(info.mode, op1, op2, pc);
        res+="$"+hexNorm(hex(pc))+":\t"+info.name+"\t" + address.first+"\n";
        length.push_back(address.second);
        pc += address.second;
    }
    return {res, length};
}

void Cpu::incrementCircular()
{
    circularIndex += 1;
    circularIndex %= 10;
}

std::pair<std::string, int> Cpu::formatInstruction(AddressMode m, uint8_t op1, uint8_t op2, uint16_t pc)
{
    switch(m){
        case ADDR_IMPLICIT:{
            return {"",1};
            break;}
        case ADDR_ACCUMULATOR:{
            return {"A",1};
            break;}
        case ADDR_IMMEDIATE:{
            return {"#$"+hex(op1),2};
            break;}
        case ADDR_ZERO_PAGE:{
            return {"$"+hex(op1),2};
            break;}
        case ADDR_ABSOLUTE:{
            uint16_t addr = (uint16_t)op1 | ((uint16_t)op2 << 8);
            return {"$"+hex(addr),3};
            break;}
        case ADDR_RELATIVE:{
            int8_t rel;
            std::memcpy(&rel, &op1, sizeof(rel));
            uint16_t loc = pc + 2 + rel;
            return {"$"+hex(loc),2};
            break;}
        case ADDR_INDIRECT:{
            uint16_t addr = (uint16_t)op1 | ((uint16_t)op2 << 8);
            return {"($"+hex(addr)+")",3};
            break;}
        case ADDR_ZERO_PAGE_INDEXED_X:{
            return {"$"+hex(op1)+",X",2};
            break;}
        case ADDR_ZERO_PAGE_INDEXED_Y:{
            return {"$"+hex(op1)+",Y",2};
            break;}
        case ADDR_ABSOLUTE_INDEXED_X:{
            uint16_t addr = (uint16_t)op1| ((uint16_t)op2 << 8);
            return {"$"+hex(addr)+",X",3};
            break;}
        case ADDR_ABSOLUTE_INDEXED_Y:{
            uint16_t addr = (uint16_t)op1 | ((uint16_t)op2 << 8);
            return {"$"+hex(addr)+",Y",3};
            break;}
        case ADDR_INDEXED_INDIRECT_X:{
            return {"($"+hex(op1)+",X)",2};
            break;}
        case ADDR_INDEXED_INDIRECT_Y:{
            return {"($"+hex(op2)+"),Y",2};
            break;}
        default:{
            return {"",1};}
    }
}

uint8_t *Cpu::getMemoryAddress(AddressMode mode, uint8_t &cycles)
{
    switch(mode){
        case ADDR_IMPLICIT:
            {cycles += 2;
            PC += 1;
            return &A;} // Sollte kein Problem darstellen, da keine Instruktion Accumulator und Implicit besitzt
                        // Muss aber so zurückgegeben, weil manche Opcodes falsch Klassifiziert sind
        case ADDR_ACCUMULATOR:
            {cycles += 2;
            PC += 1;
            return &A;}
        case ADDR_IMMEDIATE:
            {cycles += 2;
            uint8_t* ret = (uint8_t*)(uintptr_t)PC + 1;
            PC += 2;
            return ret;}
        case ADDR_ZERO_PAGE:
            {cycles += 3;
            uint8_t* ret = (uint8_t*)(uintptr_t)read((uint8_t*)(uintptr_t)PC + 1);
            PC += 2;
            return ret;}
        case ADDR_ABSOLUTE: // little endian
            {uint8_t lower = read((uint8_t*)(uintptr_t)PC + 1);
            uint8_t higher = read((uint8_t*)(uintptr_t)PC + 2);
            uint16_t addr = (((uint16_t)higher) << 8) | (uint16_t)lower;
            cycles += 4;
            PC += 3;
            return (uint8_t*)(uintptr_t)addr;}
        case ADDR_RELATIVE:
            {uint8_t arg = read((uint8_t*)(uintptr_t)PC + 1);
            int8_t offset =*((int8_t*)&arg); //ist das korrekt??
            cycles += 2;
            // if(willCrossPage(reinterpret_cast<uintptr_t>(opcode_start), offset)) // NUR FALLS BRANCH GENOMMEN, also in Instruktion machen
            //     cycles += 1;
            PC += 2;
            uint8_t* ret = (uint8_t*)((uintptr_t)offset);
            return ret;}   //PC muss gesetzt werden. von Instruktion?
        case ADDR_INDIRECT:
            {uint8_t olower = read((uint8_t*)(uintptr_t)PC + 1);
            uint8_t ohigher = read((uint8_t*)(uintptr_t)PC + 2);
            uint16_t addr = ((uint16_t)ohigher << 8) | (uint16_t)olower;
            uint8_t lower = read((uint8_t*)(uintptr_t)addr);
            uint8_t higher;
            if(olower == 0xFF) // CPU-Bug bei JMP
                higher = read((uint8_t*)(uintptr_t)(addr & 0xFF00));
            else higher = read((uint8_t*)(uintptr_t)addr + 1);
            
            addr = ((uint16_t)higher << 8) | (uint16_t)lower;
            cycles += 5;
            PC += 3;
            return (uint8_t*)(uintptr_t)addr;}
        case ADDR_INDEXED_INDIRECT_Y:
            {uint8_t zeroAddr = read((uint8_t*)(uintptr_t)PC + 1);
            uint8_t lower = read((uint8_t*)(uintptr_t)zeroAddr);
            uint8_t zeroAddrh = zeroAddr + 1;
            uint8_t higher = read((uint8_t*)(uintptr_t)zeroAddrh);
            uint16_t origAddr = (((uint16_t)higher << 8) | (uint16_t)lower);
            uint16_t addr = origAddr + ((uint16_t) Y);
            cycles += 5;
            PC += 2;
            if(willCrossPage((uintptr_t)origAddr, (uintptr_t)Y))
                cycles += 1;
            return (uint8_t*)(uintptr_t)addr;}
        case ADDR_INDEXED_INDIRECT_X:
            {uint8_t sum = read((uint8_t*)(uintptr_t)PC + 1) + X;
            uint8_t lower = read((uint8_t*)(uintptr_t)sum);
            uint8_t sumh = sum + 1;
            uint8_t higher = read((uint8_t*)(uintptr_t)sumh);
            uint16_t addr = ((uint16_t)higher << 8) | (uint16_t)lower;
            cycles += 6;
            PC += 2;
            return (uint8_t*)(uintptr_t)addr;}
        case ADDR_ZERO_PAGE_INDEXED_X:
            {uint8_t arg = read((uint8_t*)(uintptr_t)PC + 1);
            uint8_t new_addr = arg + X;
            cycles += 4;
            PC += 2;
            return (uint8_t*)(uintptr_t)new_addr;}
        case ADDR_ZERO_PAGE_INDEXED_Y:
            {uint8_t arg = read((uint8_t*)(uintptr_t)PC + 1);
            uint8_t new_addr = arg + Y;
            cycles += 4;
            PC += 2;
            return (uint8_t*)(uintptr_t)new_addr;}
        case ADDR_ABSOLUTE_INDEXED_X:
            {uint8_t lower = read((uint8_t*)(uintptr_t)PC + 1);
            uint8_t higher = read((uint8_t*)(uintptr_t)PC + 2);
            uint16_t addr = ((uint16_t)higher << 8) | (uint16_t)lower;
            cycles += 4;
            PC += 3;
            if(willCrossPage((uintptr_t)addr, (uintptr_t)X)){
                cycles += 1;
                read((uint8_t*)(uintptr_t)addr); // dummy read
            }
            uint16_t sum = addr + X; // Summe wegen 16-bit Überlauf, landet dann in der zero-page
            return (uint8_t*)((intptr_t)sum);}
        case ADDR_ABSOLUTE_INDEXED_Y:
            {uint8_t lower = read((uint8_t*)(intptr_t)PC + 1);
            uint8_t higher = read((uint8_t*)(intptr_t)PC + 2);
            uint16_t addr = ((uint16_t)higher << 8) | (uint16_t)lower;
            cycles += 4;
            PC += 3;
            if(willCrossPage((uintptr_t)addr, (uintptr_t)Y)){
                cycles += 1;
                read((uint8_t*)(uintptr_t)addr); // dummy read
            }
            uint16_t sum = addr + Y;
            return (uint8_t*)((intptr_t)sum);}
    }
    return nullptr;
}

uint8_t Cpu::executeNextInstruction()
{
    uint8_t opcode = read((uint8_t*) (intptr_t) PC);
    const opcode_info &info = opcodes[opcode];
    // std::cout << "FFFA (NMI): " << hex(read((uint8_t*)(uintptr_t)0xFFFA)) << std::endl;
    // std::cout << "FFFB (?): " << hex(read((uint8_t*)(uintptr_t)0xFFFB)) << std::endl;
    // std::cout << "FFFC (RESET): " << hex(read((uint8_t*)(uintptr_t)0xFFFC)) << std::endl;
    // std::cout << "FFFD (?): " << hex(read((uint8_t*)(uintptr_t)0xFFFD)) << std::endl;
    // std::cout << "FFFE (IRQ / BRK): " << hex(read((uint8_t*)(uintptr_t)0xFFFE)) << std::endl;
    //nestest debugging
    // assert(log.good());
    // log << std::setw(4) << std::setfill('0') << hex(PC)
    // << ", Opcode: " << std::setw(2) << std::setfill('0') << (hex(opcode))
    // << ", m: " << std::setw(2) << std::setfill('0') << info.mode 
    // << " A:" << std::setw(2) << std::setfill('0') << hex(A) 
    // << " X:" << std::setw(2) << std::setfill('0') << hex(X)
    // << " Y:" << std::setw(2) << std::setfill('0') << hex(Y)
    // << " P:" << std::setw(2) << std::setfill('0') << hex(P)
    // << " SP:" << std::setw(2) << std::setfill('0') << hex(SP)
    // << " CYC:" << totalCycles << std::endl;
    
    if(setInterruptNextInstruction.first){
        setStatus(STATUS_INTERRUPT_DISABLE, setInterruptNextInstruction.second);
        setInterruptNextInstruction = {false, false};
    }
    
    uint8_t cycles = 0;
    uint8_t* addr = getMemoryAddress(info.mode, cycles);
    uint8_t modCycles = (this->*info.instruction)(addr);
    if(info.cycles > 0)
    cycles = info.cycles; // Overwrite für "spezielle" Instruktionen, wie ASL
    else cycles += modCycles; // Für relative Adressierung, die davon abhängt, ob ein Branch genommen wird
    
    totalCycles += cycles;
    
    return cycles;
}

bool Cpu::clockCPU()
{
    bool res = false;
    if(remainingCycles<=0){

        //Debugging
        circular[circularIndex] = PC;
        incrementCircular();

        if(!setInterruptNextInstruction.first)
            remainingCycles += pollInterrupts();
        if(remainingCycles)
            return true;
        remainingCycles += executeNextInstruction();
        res = true;
    }
    remainingCycles--;
    return res;
}

// Logik prüfen
uint8_t Cpu::pollInterrupts()
{
    if(IRQgenerated || NMIgenerated){
        if(NMIgenerated){
            NMI();
            NMIWasHigh = true;
            NMIgenerated = false;
            totalCycles += 7;
            return 7;
        }
        if(IRQgenerated){
            if(!getStatus(STATUS_INTERRUPT_DISABLE)){
                IRQ();
                IRQgenerated = false;
                totalCycles += 7;
                return 7;
            }
        }
    }
    return 0;
}

void Cpu::waitFor(uint8_t cycles)
{
    remainingCycles += cycles;
}

uint8_t Cpu::read(uint8_t *p)
{ // Extrem gefährliche Lösung
    if(p==&A) return A;
    // return memory[(uintptr_t)p];
    return mapper->read(p);
}

void Cpu::write(uint8_t *p, uint8_t v)
{ // Hier auch
    if(p==&A) A = v;
    // else memory[(uintptr_t)p] = v;
    else mapper->write(p, v);
}

void Cpu::pushStack(uint8_t value)
{
    write((uint8_t*)0x0100 + SP, value);
    SP--;
}

uint8_t Cpu::pullStack()
{
    SP++;
    uint8_t result = read((uint8_t*)0x0100 + SP);
    return result;
}

uint8_t Cpu::ADC(uint8_t *mem)
{
    auto r = read(mem);
    uint16_t result = A + r + (uint8_t)getStatus(STATUS_CARRY);
    setStatus(STATUS_CARRY, result > 0xFF);
    setStatus(STATUS_ZERO, (uint8_t)result == 0); // Nur bits in A testen! deshalb cast
    setStatus(STATUS_OVERFLOW, ((uint8_t)result ^ A) & ((uint8_t)result ^ r) & 0x80);
    setStatus(STATUS_NEGATIVE, (uint8_t)result & 0b10000000);
    A = (uint8_t)result;
    return 0;
}

uint8_t Cpu::AND(uint8_t *mem)
{
    A = A & read(mem);
    setStatus(STATUS_ZERO, A == 0);
    setStatus(STATUS_NEGATIVE, A & 0b10000000);
    return 0;
}

uint8_t Cpu::ASL(uint8_t *mem)
{
    uint8_t val = read(mem);
    uint8_t result = (val << 1) & 0b11111110;
    write(mem, val);
    write(mem, result); // read-modify-write
    setStatus(STATUS_CARRY, val & 0b10000000);
    setStatus(STATUS_ZERO, result == 0);
    setStatus(STATUS_NEGATIVE, result & 0b10000000);
    return 0;
}

uint8_t Cpu::BCC(uint8_t *mem)
{
    // Ist möglicherweise nicht die sicherste Art das zu machen
    int8_t offset = reinterpret_cast<intptr_t>(mem);
    uint8_t extraCycles = 0;
    if(!getStatus(STATUS_CARRY)){
        extraCycles++;
        if(willCrossPage(PC, offset))
            extraCycles++;
        PC += offset;
    }
    return extraCycles;
}

uint8_t Cpu::BCS(uint8_t *mem)
{
    int8_t offset = reinterpret_cast<intptr_t>(mem);
    uint8_t extraCycles = 0;
    if(getStatus(STATUS_CARRY)){
        extraCycles++;
        if(willCrossPage(PC, offset))
            extraCycles++;
        PC += offset;
    }
    return extraCycles;
}

uint8_t Cpu::BEQ(uint8_t *mem)
{
    int8_t offset = reinterpret_cast<intptr_t>(mem);
    uint8_t extraCycles = 0;
    if(getStatus(STATUS_ZERO)){
        extraCycles++;
        if(willCrossPage(PC, offset))
            extraCycles++;
        PC += offset;
    }
    return extraCycles;
}

uint8_t Cpu::BIT(uint8_t *mem)
{
    // Nur ein READ!!!
    auto r = read(mem);
    uint8_t result = A & r;
    setStatus(STATUS_ZERO, result==0);
    setStatus(STATUS_OVERFLOW, r & 0b01000000);
    setStatus(STATUS_NEGATIVE, r & 0b10000000);
    return 0;
}

uint8_t Cpu::BMI(uint8_t *mem)
{
    int8_t offset = reinterpret_cast<intptr_t>(mem);
    uint8_t extraCycles = 0;
    if(getStatus(STATUS_NEGATIVE)){
        extraCycles++;
        if(willCrossPage(PC, offset))
            extraCycles++;
        PC += offset;
    }
    return extraCycles;
}

uint8_t Cpu::BNE(uint8_t *mem)
{
    int8_t offset = reinterpret_cast<intptr_t>(mem);
    uint8_t extraCycles = 0;
    if(!getStatus(STATUS_ZERO)){
        extraCycles++;
        if(willCrossPage(PC, offset))
            extraCycles++;
        PC += offset;
    }
    return extraCycles;
}

uint8_t Cpu::BPL(uint8_t *mem)
{
    int8_t offset = reinterpret_cast<intptr_t>(mem);
    uint8_t extraCycles = 0;
    if(!getStatus(STATUS_NEGATIVE)){
        extraCycles++;
        if(willCrossPage(PC, offset))
            extraCycles++;
        PC += offset;
    }
    return extraCycles;
}

uint8_t Cpu::BRK(uint8_t *mem)
{
    uint16_t PCH = (PC >> 8) & 0b0000000011111111;
    pushStack(PCH);
    pushStack(PC);
    pushStack(P | 0b00010000); // B gesetzt
    uint8_t lower = read((uint8_t*)0xFFFE);
    uint8_t higher = read((uint8_t*)0xFFFF);
    uint16_t addr = ((uint16_t)higher << 8) | (uint16_t)lower;
    PC = addr;
    setStatus(STATUS_INTERRUPT_DISABLE, true);
    std::cout << "BRK wurde ausgelöst!" << std::endl;
    return 0;
}

uint8_t Cpu::BVC(uint8_t *mem)
{
    int8_t offset = reinterpret_cast<intptr_t>(mem);
    uint8_t extraCycles = 0;
    if(!getStatus(STATUS_OVERFLOW)){
        extraCycles++;
        if(willCrossPage(PC, offset))
            extraCycles++;
        PC += offset;
    }
    return extraCycles;
}

uint8_t Cpu::BVS(uint8_t *mem)
{
    int8_t offset = reinterpret_cast<intptr_t>(mem);
    uint8_t extraCycles = 0;
    if(getStatus(STATUS_OVERFLOW)){
        extraCycles++;
        if(willCrossPage(PC, offset))
            extraCycles++;
        PC += offset;
    }
    return extraCycles;
}

uint8_t Cpu::CLC(uint8_t *mem)
{
    setStatus(STATUS_CARRY, false);
    return 0;
}

uint8_t Cpu::CLD(uint8_t *mem)
{
    setStatus(STATUS_DECIMAL, false);
    return 0;
}

uint8_t Cpu::CLI(uint8_t *mem)
{
    setInterruptNextInstruction = {true, false};
    return 0;
}

uint8_t Cpu::CLV(uint8_t *mem)
{
    setStatus(STATUS_OVERFLOW, false);
    return 0;
}

uint8_t Cpu::CMP(uint8_t *mem)
{
    auto r = read(mem);
    uint8_t result = A - r;
    setStatus(STATUS_CARRY, A >= r);
    setStatus(STATUS_ZERO, A == r);
    setStatus(STATUS_NEGATIVE, result & 0b10000000);
    return 0;
}

uint8_t Cpu::CPX(uint8_t *mem)
{
    auto r = read(mem);
    uint8_t result = X - r;
    setStatus(STATUS_CARRY, X >= r);
    setStatus(STATUS_ZERO, X == r);
    setStatus(STATUS_NEGATIVE, result & 0b10000000);
    return 0;
}

uint8_t Cpu::CPY(uint8_t *mem)
{
    auto r = read(mem);
    uint8_t result = Y - r;
    setStatus(STATUS_CARRY, Y >= r);
    setStatus(STATUS_ZERO, Y == r);
    setStatus(STATUS_NEGATIVE, result & 0b10000000);
    return 0;
}

uint8_t Cpu::DEC(uint8_t *mem)
{
    uint8_t val = read(mem);
    uint8_t result = val - 1;
    write(mem, val); // read-modify-write
    write(mem, result);
    setStatus(STATUS_ZERO, result == 0);
    setStatus(STATUS_NEGATIVE, result & 0b10000000);
    return 0;
}

uint8_t Cpu::DEX(uint8_t *mem)
{
    X--;
    setStatus(STATUS_ZERO, X == 0);
    setStatus(STATUS_NEGATIVE, X & 0b10000000);
    return 0;
}

uint8_t Cpu::DEY(uint8_t *mem)
{
    Y--;
    setStatus(STATUS_ZERO, Y == 0);
    setStatus(STATUS_NEGATIVE, Y & 0b10000000);
    return 0;
}

uint8_t Cpu::EOR(uint8_t *mem)
{
    A = A ^ read(mem);
    setStatus(STATUS_ZERO, A == 0);
    setStatus(STATUS_NEGATIVE, A & 0b10000000);
    return 0;
}

uint8_t Cpu::INC(uint8_t *mem)
{
    uint8_t val = read(mem);
    uint8_t result = val + 1;
    write(mem, val); // read-modify-write
    write(mem, result);
    setStatus(STATUS_ZERO, result == 0);
    setStatus(STATUS_NEGATIVE, result & 0b10000000);
    return 0;
}

uint8_t Cpu::INX(uint8_t *mem)
{
    X++;
    setStatus(STATUS_ZERO, X == 0);
    setStatus(STATUS_NEGATIVE, X & 0b10000000);
    return 0;
}

uint8_t Cpu::INY(uint8_t *mem)
{
    Y++;
    setStatus(STATUS_ZERO, Y == 0);
    setStatus(STATUS_NEGATIVE, Y & 0b10000000);
    return 0;
}

uint8_t Cpu::JMP(uint8_t *mem)
{
    PC = (uintptr_t)(mem); // Falsch?
    return 0;
}

uint8_t Cpu::JSR(uint8_t *mem)
{
    pushStack((PC-1) >> 8);
    pushStack(PC-1);
    PC = (uintptr_t)mem;
    return 0;
}

uint8_t Cpu::LDA(uint8_t *mem)
{
    A = read(mem);
    setStatus(STATUS_ZERO, A == 0);
    setStatus(STATUS_NEGATIVE, A & 0b10000000);
    return 0;
}

uint8_t Cpu::LDX(uint8_t *mem)
{
    X = read(mem);
    setStatus(STATUS_ZERO, X == 0);
    setStatus(STATUS_NEGATIVE, X & 0b10000000);
    return 0;
}

uint8_t Cpu::LDY(uint8_t *mem)
{
    Y = read(mem);
    setStatus(STATUS_ZERO, Y == 0);
    setStatus(STATUS_NEGATIVE, Y & 0b10000000);
    return 0;
}

uint8_t Cpu::LSR(uint8_t *mem)
{
    uint8_t val = read(mem);
    uint8_t result = (val >> 1) & 0b01111111;
    write(mem, val);
    write(mem, result);
    setStatus(STATUS_CARRY, val & 0b00000001);
    setStatus(STATUS_ZERO, result == 0);
    setStatus(STATUS_NEGATIVE, false);
    return 0;
}

uint8_t Cpu::NOP(uint8_t *mem)
{
    return 0;
}

uint8_t Cpu::ORA(uint8_t *mem)
{
    A = A | read(mem);
    setStatus(STATUS_ZERO, A == 0);
    setStatus(STATUS_NEGATIVE, A & 0b10000000);
    return 0;
}

uint8_t Cpu::PHA(uint8_t *mem)
{
    write((uint8_t*)0x0100 + SP, A);
    SP--;
    return 0;
}

uint8_t Cpu::PHP(uint8_t *mem)
{
    write((uint8_t*)0x0100 + SP, P | 0b00110000);
    SP--;
    return 0;
}

uint8_t Cpu::PLA(uint8_t *mem)
{
    SP++;
    A = read((uint8_t*)0x0100 + SP);
    setStatus(STATUS_ZERO, A == 0);
    setStatus(STATUS_NEGATIVE, A & 0b10000000);
    return 0;
}

uint8_t Cpu::PLP(uint8_t *mem)
{
    SP++;
    uint8_t val = read((uint8_t*)0x0100 + SP);
    setStatus(STATUS_CARRY,    val & 0b00000001);
    setStatus(STATUS_ZERO,     val & 0b00000010);
    setInterruptNextInstruction = {true, val & 0b00000100};
    setStatus(STATUS_DECIMAL,  val & 0b00001000);
    setStatus(STATUS_OVERFLOW, val & 0b01000000);
    setStatus(STATUS_NEGATIVE, val & 0b10000000);
    return 0;
}

uint8_t Cpu::ROL(uint8_t *mem)
{
    uint8_t val = read(mem);
    uint8_t result = ((val << 1 ) & 0b11111110) | getStatus(STATUS_CARRY);
    write(mem, val);
    write(mem, result);
    setStatus(STATUS_CARRY,    val & 0b10000000);
    setStatus(STATUS_ZERO,     result == 0);
    setStatus(STATUS_NEGATIVE, result & 0b10000000);
    return 0;
}

uint8_t Cpu::ROR(uint8_t *mem)
{
    uint8_t val = read(mem);
    uint8_t result = ((val >> 1 ) & 0b01111111) | (getStatus(STATUS_CARRY) << 7);
    write(mem, val);
    write(mem, result);
    setStatus(STATUS_CARRY,    val & 0b00000001);
    setStatus(STATUS_ZERO,     result == 0);
    setStatus(STATUS_NEGATIVE, result & 0b10000000);
    return 0;
}

uint8_t Cpu::RTI(uint8_t *mem)
{
    uint8_t val = pullStack();
    uint8_t PCl = pullStack();
    uint8_t PCh = pullStack();
    PC = ((uint16_t)PCh << 8) | PCl;
    setStatus(STATUS_CARRY,                 val & 0b00000001);
    setStatus(STATUS_ZERO,                  val & 0b00000010);
    setStatus(STATUS_INTERRUPT_DISABLE,     val & 0b00000100);
    setStatus(STATUS_DECIMAL,               val & 0b00001000);
    setStatus(STATUS_OVERFLOW,              val & 0b01000000);
    setStatus(STATUS_NEGATIVE,              val & 0b10000000);
    return 0;
}

uint8_t Cpu::RTS(uint8_t *mem)
{
    uint8_t PCl = pullStack();
    uint8_t PCh = pullStack();
    PC = ((uint16_t)PCh << 8) | PCl;
    PC += 1;
    return 0;
}

uint8_t Cpu::SBC(uint8_t *mem)
{
    uint8_t val = read(mem);
    uint8_t result = A + ~val + ((uint8_t)getStatus(STATUS_CARRY));
    int intres = A + ~val + ((uint8_t)getStatus(STATUS_CARRY));
    setStatus(STATUS_CARRY, !(intres < 0)); // blöd, ja, aber einfach
    setStatus(STATUS_ZERO, result == 0);
    setStatus(STATUS_OVERFLOW, (result ^ A) & (result ^ ~val) & 0x80);
    setStatus(STATUS_NEGATIVE, result & 0b10000000);
    A = result;
    return 0;
}

uint8_t Cpu::SEC(uint8_t *mem)
{
    setStatus(STATUS_CARRY, true);
    return 0;
}

uint8_t Cpu::SED(uint8_t *mem)
{
    setStatus(STATUS_DECIMAL, true);
    return 0;
}

uint8_t Cpu::SEI(uint8_t *mem)
{
    setInterruptNextInstruction = {true, true};
    return 0;
}

uint8_t Cpu::STA(uint8_t *mem)
{
    write(mem, A);
    return 0;
}

uint8_t Cpu::STX(uint8_t *mem)
{
    write(mem, X);
    return 0;
}

uint8_t Cpu::STY(uint8_t *mem)
{
    write(mem, Y);
    return 0;
}

uint8_t Cpu::TAX(uint8_t *mem)
{
    X = A;
    setStatus(STATUS_ZERO, X == 0);
    setStatus(STATUS_NEGATIVE, X & 0b10000000);
    return 0;
}

uint8_t Cpu::TAY(uint8_t *mem)
{
    Y = A;
    setStatus(STATUS_ZERO, Y == 0);
    setStatus(STATUS_NEGATIVE, Y & 0b10000000);
    return 0;
}

uint8_t Cpu::TSX(uint8_t *mem)
{
    X = SP;
    setStatus(STATUS_ZERO, X == 0);
    setStatus(STATUS_NEGATIVE, X & 0b10000000);
    return 0;
}

uint8_t Cpu::TXA(uint8_t *mem)
{
    A = X;
    setStatus(STATUS_ZERO, A == 0);
    setStatus(STATUS_NEGATIVE, A & 0b10000000);
    return 0;
}

uint8_t Cpu::TXS(uint8_t *mem)
{
    SP = X;
    return 0;
}

uint8_t Cpu::TYA(uint8_t *mem)
{
    A = Y;
    setStatus(STATUS_ZERO, A == 0);
    setStatus(STATUS_NEGATIVE, A & 0b10000000);
    return 0;
}

uint8_t Cpu::STP(uint8_t *mem)
{
    std::cout << "NICHT IMPLEMENTIERT"  << std::endl;
    return 0;
}

uint8_t Cpu::SLO(uint8_t *mem)
{
    ASL(mem);
    ORA(mem);
    return 0;
}

uint8_t Cpu::ANC(uint8_t *mem)
{
    A = A & read(mem);
    setStatus(STATUS_ZERO, A == 0);
    setStatus(STATUS_NEGATIVE, A & 0b10000000);
    setStatus(STATUS_CARRY, A & 0b10000000);
    return 0;
}

uint8_t Cpu::RLA(uint8_t *mem)
{
    ROL(mem);
    AND(mem);
    return 0;
}

uint8_t Cpu::SRE(uint8_t *mem)
{
    LSR(mem);
    EOR(mem);
    return 0;
}

uint8_t Cpu::ALR(uint8_t *mem)
{
    uint8_t val = A & read(mem);
    A = val;
    A >>= 1;
    setStatus(STATUS_CARRY, val & 0b00000001);
    setStatus(STATUS_ZERO, A == 0);
    setStatus(STATUS_NEGATIVE, false);
    return 0;
}

uint8_t Cpu::RRA(uint8_t *mem)
{
    ROR(mem);
    ADC(mem);
    return 0;
}

uint8_t Cpu::ARR(uint8_t *mem)
{
    A = A & read(mem);
    A = (A >> 1) | (getStatus(STATUS_CARRY) ? 0x80 : 0);
    setStatus(STATUS_NEGATIVE, A & 0b10000000);
    setStatus(STATUS_ZERO, A == 0);
    switch((A & 0b01100000) >> 5){
        case 3:
            setStatus(STATUS_CARRY, true);
            setStatus(STATUS_OVERFLOW, false);
            break;
        case 2:
            setStatus(STATUS_OVERFLOW, true);
            setStatus(STATUS_CARRY, true);
            break;
        case 1:
            setStatus(STATUS_OVERFLOW, true);
            setStatus(STATUS_CARRY, false);
            break;
        default:
            setStatus(STATUS_OVERFLOW, false);
            setStatus(STATUS_CARRY, false);
            break;
    }
    return 0;
}

uint8_t Cpu::SAX(uint8_t *mem)
{
    write(mem, A & X);
    return 0;
}

uint8_t Cpu::XAA(uint8_t *mem)
{
    std::cout << "NICHT IMPLEMENTIERT"  << std::endl;
    return 0;
}

uint8_t Cpu::AHX(uint8_t *mem)
{
    std::cout << "Instabile, Fehlerhafte Operation!"  << std::endl;
    write(mem, (A & X) & ( 1 + (((uint16_t)(uintptr_t)mem) >> 8)));
    return 0;
}

uint8_t Cpu::TAS(uint8_t *mem)
{
    std::cout << "NICHT IMPLEMENTIERT"  << std::endl;
    return 0;
}

uint8_t Cpu::SHY(uint8_t *mem)
{
    std::cout << "Instabile, Fehlerhafte Operation!"  << std::endl;
    write(mem, Y & ( 1 + (((uint16_t)(uintptr_t)mem) >> 8)));
    return 1;
}

uint8_t Cpu::SHX(uint8_t *mem)
{
    std::cout << "Instabile, Fehlerhafte Operation!"  << std::endl;
    write(mem, X & ( 1 + (((uint16_t)(uintptr_t)mem) >> 8)));
    return 1;
}

uint8_t Cpu::LAX(uint8_t *mem)
{
    LDA(mem);
    LDX(mem);
    return 0;
}

uint8_t Cpu::LAS(uint8_t *mem)
{
    std::cout << "NICHT IMPLEMENTIERT"  << std::endl;
    return 0;
}

uint8_t Cpu::DCP(uint8_t *mem)
{
    DEC(mem);
    CMP(mem);
    return 0;
}

uint8_t Cpu::AXS(uint8_t *mem)
{
    uint8_t val = read(mem);
    uint8_t res = (A & X) - val;
    setStatus(STATUS_CARRY, (A & X) >= val);
    setStatus(STATUS_ZERO, (A & X) == val);
    setStatus(STATUS_NEGATIVE, res & 0b10000000);
    X = res;
    return 2;
}

uint8_t Cpu::ISC(uint8_t *mem)
{
    INC(mem);
    SBC(mem);
    return 0;
}
