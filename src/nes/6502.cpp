#include "6502.h"
#include <limits>
#include <format>
#include <string>
#include <cstring>
#include "../framework/stringlib.h"

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
        uint8_t opcode = read((uint8_t*)(uintptr_t)pc);
        uint8_t op1 = read((uint8_t*)(uintptr_t)pc+1);
        uint8_t op2 = read((uint8_t*)(uintptr_t)pc+2);
        const opcode_info &info = opcodes[opcode];

        const std::pair<std::string, int> &address = formatInstruction(info.mode, op1, op2, pc);
        res+=getHexDollar(pc, 4)+":\t"+info.name+"\t" + address.first+"\n";
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
        res+=getHexDollar(pc, 4)+":\t"+info.name+"\t" + address.first+"\n";
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

void Cpu::unimplemented(const std::string &instruction)
{
    #ifdef BUILD_DESKTOP
    MessageStruct m{
        .type = MT_ERROR,
        .title = locale.getTranslation(EmulatorError),
        .content = std::vformat(locale.getTranslation(UnimplementedInstruction), std::make_format_args(instruction))
    };
    messageQueue.enqueue(m);
    #else
    (void)instruction;
    #endif
}

void Cpu::falseImplementation(const std::string &instruction)
{
    #ifdef BUILD_DESKTOP
    MessageStruct m{
        .type = MT_WARNING,
        .title = locale.getTranslation(EmulatorError),
        .content = std::vformat(locale.getTranslation(UnsafeInstruction), std::make_format_args(instruction))
    };
    messageQueue.enqueue(m);
    #else
    (void)instruction;
    #endif
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
    (void)mem;
    uint16_t PCH = (PC >> 8) & 0b0000000011111111;
    pushStack(PCH);
    pushStack(PC);
    pushStack(P | 0b00010000); // B gesetzt
    uint8_t lower = read((uint8_t*)0xFFFE);
    uint8_t higher = read((uint8_t*)0xFFFF);
    uint16_t addr = ((uint16_t)higher << 8) | (uint16_t)lower;
    PC = addr;
    setStatus(STATUS_INTERRUPT_DISABLE, true);


    #ifdef BUILD_DESKTOP
    MessageStruct m{
        .type = MT_WARNING,
        .title = locale.getTranslation(EmulatorWarning),
        .content = locale.getTranslation(BRKExecuted)
    };
    messageQueue.enqueue(m);
    #endif

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
    (void)mem;
    setStatus(STATUS_CARRY, false);
    return 0;
}

uint8_t Cpu::CLD(uint8_t *mem)
{
    (void)mem;
    setStatus(STATUS_DECIMAL, false);
    return 0;
}

uint8_t Cpu::CLI(uint8_t *mem)
{
    (void)mem;
    setInterruptNextInstruction = {true, false};
    return 0;
}

uint8_t Cpu::CLV(uint8_t *mem)
{
    (void)mem;
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
    (void)mem;
    X--;
    setStatus(STATUS_ZERO, X == 0);
    setStatus(STATUS_NEGATIVE, X & 0b10000000);
    return 0;
}

uint8_t Cpu::DEY(uint8_t *mem)
{
    (void)mem;
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
    (void)mem;
    X++;
    setStatus(STATUS_ZERO, X == 0);
    setStatus(STATUS_NEGATIVE, X & 0b10000000);
    return 0;
}

uint8_t Cpu::INY(uint8_t *mem)
{
    (void)mem;
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
    (void)mem;
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
    (void)mem;
    write((uint8_t*)0x0100 + SP, A);
    SP--;
    return 0;
}

uint8_t Cpu::PHP(uint8_t *mem)
{
    (void)mem;
    write((uint8_t*)0x0100 + SP, P | 0b00110000);
    SP--;
    return 0;
}

uint8_t Cpu::PLA(uint8_t *mem)
{
    (void)mem;
    SP++;
    A = read((uint8_t*)0x0100 + SP);
    setStatus(STATUS_ZERO, A == 0);
    setStatus(STATUS_NEGATIVE, A & 0b10000000);
    return 0;
}

uint8_t Cpu::PLP(uint8_t *mem)
{
    (void)mem;
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
    (void)mem;
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
    (void)mem;
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
    (void)mem;
    setStatus(STATUS_CARRY, true);
    return 0;
}

uint8_t Cpu::SED(uint8_t *mem)
{
    (void)mem;
    setStatus(STATUS_DECIMAL, true);
    return 0;
}

uint8_t Cpu::SEI(uint8_t *mem)
{
    (void)mem;
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
    (void)mem;
    X = A;
    setStatus(STATUS_ZERO, X == 0);
    setStatus(STATUS_NEGATIVE, X & 0b10000000);
    return 0;
}

uint8_t Cpu::TAY(uint8_t *mem)
{
    (void)mem;
    Y = A;
    setStatus(STATUS_ZERO, Y == 0);
    setStatus(STATUS_NEGATIVE, Y & 0b10000000);
    return 0;
}

uint8_t Cpu::TSX(uint8_t *mem)
{
    (void)mem;
    X = SP;
    setStatus(STATUS_ZERO, X == 0);
    setStatus(STATUS_NEGATIVE, X & 0b10000000);
    return 0;
}

uint8_t Cpu::TXA(uint8_t *mem)
{
    (void)mem;
    A = X;
    setStatus(STATUS_ZERO, A == 0);
    setStatus(STATUS_NEGATIVE, A & 0b10000000);
    return 0;
}

uint8_t Cpu::TXS(uint8_t *mem)
{
    (void)mem;
    SP = X;
    return 0;
}

uint8_t Cpu::TYA(uint8_t *mem)
{
    (void)mem;
    A = Y;
    setStatus(STATUS_ZERO, A == 0);
    setStatus(STATUS_NEGATIVE, A & 0b10000000);
    return 0;
}

uint8_t Cpu::STP(uint8_t *mem)
{
    (void)mem;
    unimplemented("STP");
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
    (void)mem;
    unimplemented("XAA");
    return 0;
}

uint8_t Cpu::AHX(uint8_t *mem)
{
    falseImplementation("AHX");
    write(mem, (A & X) & ( 1 + (((uint16_t)(uintptr_t)mem) >> 8)));
    return 0;
}

uint8_t Cpu::TAS(uint8_t *mem)
{
    (void)mem;
    unimplemented("TAS");
    return 0;
}

uint8_t Cpu::SHY(uint8_t *mem)
{
    falseImplementation("SHY");
    write(mem, Y & ( 1 + (((uint16_t)(uintptr_t)mem) >> 8)));
    return 1;
}

uint8_t Cpu::SHX(uint8_t *mem)
{
    falseImplementation("SHX");
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
    (void)mem;
    unimplemented("LAS");
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

const Cpu::opcode_info Cpu::opcodes[256] = {
        // Zyklen sind nur für Operationen, die eine nicht-normale Anzahl Zyklen benötigen
        // bzw. Implicit, aber überspringt folgendes Byte
        {"BRK", &Cpu::BRK, ADDR_IMMEDIATE, 7},				// $00
        {"ORA", &Cpu::ORA, ADDR_INDEXED_INDIRECT_X, 0},		// $01
        {"STP", &Cpu::STP, ADDR_IMPLICIT, 0},				// $02
        {"SLO", &Cpu::SLO, ADDR_INDEXED_INDIRECT_X, 8},		// $03
        {"NOP", &Cpu::NOP, ADDR_ZERO_PAGE, 0},				// $04
        {"ORA", &Cpu::ORA, ADDR_ZERO_PAGE, 0},				// $05
        {"ASL", &Cpu::ASL, ADDR_ZERO_PAGE, 5},				// $06
        {"SLO", &Cpu::SLO, ADDR_ZERO_PAGE, 5},				// $07
        {"PHP", &Cpu::PHP, ADDR_IMPLICIT, 3},				// $08
        {"ORA", &Cpu::ORA, ADDR_IMMEDIATE, 0},				// $09
        {"ASL", &Cpu::ASL, ADDR_IMPLICIT, 2},				// $0A
        {"ANC", &Cpu::ANC, ADDR_IMMEDIATE, 0},				// $0B
        {"NOP", &Cpu::NOP, ADDR_ABSOLUTE, 0},				// $0C
        {"ORA", &Cpu::ORA, ADDR_ABSOLUTE, 0},				// $0D
        {"ASL", &Cpu::ASL, ADDR_ABSOLUTE, 6},				// $0E
        {"SLO", &Cpu::SLO, ADDR_ABSOLUTE, 6},				// $0F
        {"BPL", &Cpu::BPL, ADDR_RELATIVE, 0},				// $10
        {"ORA", &Cpu::ORA, ADDR_INDEXED_INDIRECT_Y, 0},		// $11
        {"STP", &Cpu::STP, ADDR_IMPLICIT, 0},				// $12
        {"SLO", &Cpu::SLO, ADDR_INDEXED_INDIRECT_Y, 8},		// $13
        {"NOP", &Cpu::NOP, ADDR_ZERO_PAGE_INDEXED_X, 0},	// $14
        {"ORA", &Cpu::ORA, ADDR_ZERO_PAGE_INDEXED_X, 0},	// $15
        {"ASL", &Cpu::ASL, ADDR_ZERO_PAGE_INDEXED_X, 6},	// $16
        {"SLO", &Cpu::SLO, ADDR_ZERO_PAGE_INDEXED_X, 6},	// $17
        {"CLC", &Cpu::CLC, ADDR_IMPLICIT, 0},				// $18
        {"ORA", &Cpu::ORA, ADDR_ABSOLUTE_INDEXED_Y, 0},		// $19
        {"NOP", &Cpu::NOP, ADDR_IMPLICIT, 2},				// $1A
        {"SLO", &Cpu::SLO, ADDR_ABSOLUTE_INDEXED_Y, 7},		// $1B
        {"NOP", &Cpu::NOP, ADDR_ABSOLUTE_INDEXED_X, 0},		// $1C
        {"ORA", &Cpu::ORA, ADDR_ABSOLUTE_INDEXED_X, 0},		// $1D
        {"ASL", &Cpu::ASL, ADDR_ABSOLUTE_INDEXED_X, 7},		// $1E
        {"SLO", &Cpu::SLO, ADDR_ABSOLUTE_INDEXED_X, 7},		// $1F
        {"JSR", &Cpu::JSR, ADDR_ABSOLUTE, 6},				// $20
        {"AND", &Cpu::AND, ADDR_INDEXED_INDIRECT_X, 0},		// $21
        {"STP", &Cpu::STP, ADDR_IMPLICIT, 0},				// $22
        {"RLA", &Cpu::RLA, ADDR_INDEXED_INDIRECT_X, 8},		// $23
        {"BIT", &Cpu::BIT, ADDR_ZERO_PAGE, 0},				// $24
        {"AND", &Cpu::AND, ADDR_ZERO_PAGE, 0},				// $25
        {"ROL", &Cpu::ROL, ADDR_ZERO_PAGE, 5},				// $26
        {"RLA", &Cpu::RLA, ADDR_ZERO_PAGE, 5},				// $27
        {"PLP", &Cpu::PLP, ADDR_IMPLICIT, 4},				// $28
        {"AND", &Cpu::AND, ADDR_IMMEDIATE, 0},				// $29
        {"ROL", &Cpu::ROL, ADDR_IMPLICIT, 2},				// $2A
        {"ANC", &Cpu::ANC, ADDR_IMMEDIATE, 0},				// $2B
        {"BIT", &Cpu::BIT, ADDR_ABSOLUTE, 0},				// $2C
        {"AND", &Cpu::AND, ADDR_ABSOLUTE, 0},				// $2D
        {"ROL", &Cpu::ROL, ADDR_ABSOLUTE, 6},				// $2E
        {"RLA", &Cpu::RLA, ADDR_ABSOLUTE, 6},				// $2F
        {"BMI", &Cpu::BMI, ADDR_RELATIVE, 0},				// $30
        {"AND", &Cpu::AND, ADDR_INDEXED_INDIRECT_Y, 0},		// $31
        {"STP", &Cpu::STP, ADDR_IMPLICIT, 0},				// $32
        {"RLA", &Cpu::RLA, ADDR_INDEXED_INDIRECT_Y, 8},		// $33
        {"NOP", &Cpu::NOP, ADDR_ZERO_PAGE_INDEXED_X, 0},	// $34
        {"AND", &Cpu::AND, ADDR_ZERO_PAGE_INDEXED_X, 0},	// $35
        {"ROL", &Cpu::ROL, ADDR_ZERO_PAGE_INDEXED_X, 6},	// $36
        {"RLA", &Cpu::RLA, ADDR_ZERO_PAGE_INDEXED_X, 6},	// $37
        {"SEC", &Cpu::SEC, ADDR_IMPLICIT, 0},				// $38
        {"AND", &Cpu::AND, ADDR_ABSOLUTE_INDEXED_Y, 0},		// $39
        {"NOP", &Cpu::NOP, ADDR_IMPLICIT, 0},				// $3A
        {"RLA", &Cpu::RLA, ADDR_ABSOLUTE_INDEXED_Y, 7},		// $3B
        {"NOP", &Cpu::NOP, ADDR_ABSOLUTE_INDEXED_X, 0},		// $3C
        {"AND", &Cpu::AND, ADDR_ABSOLUTE_INDEXED_X, 0},		// $3D
        {"ROL", &Cpu::ROL, ADDR_ABSOLUTE_INDEXED_X, 7},		// $3E
        {"RLA", &Cpu::RLA, ADDR_ABSOLUTE_INDEXED_X, 7},		// $3F
        {"RTI", &Cpu::RTI, ADDR_IMPLICIT, 6},				// $40
        {"EOR", &Cpu::EOR, ADDR_INDEXED_INDIRECT_X, 0},		// $41
        {"STP", &Cpu::STP, ADDR_IMPLICIT, 0},				// $42
        {"SRE", &Cpu::SRE, ADDR_INDEXED_INDIRECT_X, 8},		// $43
        {"NOP", &Cpu::NOP, ADDR_ZERO_PAGE, 0},				// $44
        {"EOR", &Cpu::EOR, ADDR_ZERO_PAGE, 0},				// $45
        {"LSR", &Cpu::LSR, ADDR_ZERO_PAGE, 5},				// $46
        {"SRE", &Cpu::SRE, ADDR_ZERO_PAGE, 5},				// $47
        {"PHA", &Cpu::PHA, ADDR_IMPLICIT, 3},				// $48
        {"EOR", &Cpu::EOR, ADDR_IMMEDIATE, 0},				// $49
        {"LSR", &Cpu::LSR, ADDR_IMPLICIT, 2},				// $4A
        {"ALR", &Cpu::ALR, ADDR_IMMEDIATE, 0},				// $4B
        {"JMP", &Cpu::JMP, ADDR_ABSOLUTE, 3},				// $4C
        {"EOR", &Cpu::EOR, ADDR_ABSOLUTE, 0},				// $4D
        {"LSR", &Cpu::LSR, ADDR_ABSOLUTE, 6},				// $4E
        {"SRE", &Cpu::SRE, ADDR_ABSOLUTE, 6},				// $4F
        {"BVC", &Cpu::BVC, ADDR_RELATIVE, 0},				// $50
        {"EOR", &Cpu::EOR, ADDR_INDEXED_INDIRECT_Y, 0},		// $51
        {"STP", &Cpu::STP, ADDR_IMPLICIT, 0},				// $52
        {"SRE", &Cpu::SRE, ADDR_INDEXED_INDIRECT_Y, 8},		// $53
        {"NOP", &Cpu::NOP, ADDR_ZERO_PAGE_INDEXED_X, 0},	// $54
        {"EOR", &Cpu::EOR, ADDR_ZERO_PAGE_INDEXED_X, 0},	// $55
        {"LSR", &Cpu::LSR, ADDR_ZERO_PAGE_INDEXED_X, 6},	// $56
        {"SRE", &Cpu::SRE, ADDR_ZERO_PAGE_INDEXED_X, 6},	// $57
        {"CLI", &Cpu::CLI, ADDR_IMPLICIT, 0},				// $58
        {"EOR", &Cpu::EOR, ADDR_ABSOLUTE_INDEXED_Y, 0},		// $59
        {"NOP", &Cpu::NOP, ADDR_IMPLICIT, 0},				// $5A
        {"SRE", &Cpu::SRE, ADDR_ABSOLUTE_INDEXED_Y, 7},		// $5B
        {"NOP", &Cpu::NOP, ADDR_ABSOLUTE_INDEXED_X, 0},		// $5C
        {"EOR", &Cpu::EOR, ADDR_ABSOLUTE_INDEXED_X, 0},		// $5D
        {"LSR", &Cpu::LSR, ADDR_ABSOLUTE_INDEXED_X, 7},		// $5E
        {"SRE", &Cpu::SRE, ADDR_ABSOLUTE_INDEXED_X, 7},		// $5F
        {"RTS", &Cpu::RTS, ADDR_IMPLICIT, 6},				// $60
        {"ADC", &Cpu::ADC, ADDR_INDEXED_INDIRECT_X, 0},		// $61
        {"STP", &Cpu::STP, ADDR_IMPLICIT, 0},				// $62
        {"RRA", &Cpu::RRA, ADDR_INDEXED_INDIRECT_X, 8},		// $63
        {"NOP", &Cpu::NOP, ADDR_ZERO_PAGE, 0},				// $64
        {"ADC", &Cpu::ADC, ADDR_ZERO_PAGE, 0},				// $65
        {"ROR", &Cpu::ROR, ADDR_ZERO_PAGE, 5},				// $66
        {"RRA", &Cpu::RRA, ADDR_ZERO_PAGE, 5},				// $67
        {"PLA", &Cpu::PLA, ADDR_IMPLICIT, 4},				// $68
        {"ADC", &Cpu::ADC, ADDR_IMMEDIATE, 0},				// $69
        {"ROR", &Cpu::ROR, ADDR_IMPLICIT, 2},				// $6A
        {"ARR", &Cpu::ARR, ADDR_IMMEDIATE, 0},				// $6B
        {"JMP", &Cpu::JMP, ADDR_INDIRECT, 5},				// $6C
        {"ADC", &Cpu::ADC, ADDR_ABSOLUTE, 0},				// $6D
        {"ROR", &Cpu::ROR, ADDR_ABSOLUTE, 6},				// $6E
        {"RRA", &Cpu::RRA, ADDR_ABSOLUTE, 6},				// $6F
        {"BVS", &Cpu::BVS, ADDR_RELATIVE, 0},				// $70
        {"ADC", &Cpu::ADC, ADDR_INDEXED_INDIRECT_Y, 0},		// $71
        {"STP", &Cpu::STP, ADDR_IMPLICIT, 0},				// $72
        {"RRA", &Cpu::RRA, ADDR_INDEXED_INDIRECT_Y, 8},		// $73
        {"NOP", &Cpu::NOP, ADDR_ZERO_PAGE_INDEXED_X, 0},	// $74
        {"ADC", &Cpu::ADC, ADDR_ZERO_PAGE_INDEXED_X, 0},	// $75
        {"ROR", &Cpu::ROR, ADDR_ZERO_PAGE_INDEXED_X, 6},	// $76
        {"RRA", &Cpu::RRA, ADDR_ZERO_PAGE_INDEXED_X, 6},	// $77
        {"SEI", &Cpu::SEI, ADDR_IMPLICIT, 0},				// $78
        {"ADC", &Cpu::ADC, ADDR_ABSOLUTE_INDEXED_Y, 0},		// $79
        {"NOP", &Cpu::NOP, ADDR_IMPLICIT, 0},				// $7A
        {"RRA", &Cpu::RRA, ADDR_ABSOLUTE_INDEXED_Y, 7},		// $7B
        {"NOP", &Cpu::NOP, ADDR_ABSOLUTE_INDEXED_X, 0},		// $7C
        {"ADC", &Cpu::ADC, ADDR_ABSOLUTE_INDEXED_X, 0},		// $7D
        {"ROR", &Cpu::ROR, ADDR_ABSOLUTE_INDEXED_X, 7},		// $7E
        {"RRA", &Cpu::RRA, ADDR_ABSOLUTE_INDEXED_X, 7},		// $7F
        {"NOP", &Cpu::NOP, ADDR_IMMEDIATE, 0},				// $80
        {"STA", &Cpu::STA, ADDR_INDEXED_INDIRECT_X, 6},		// $81
        {"NOP", &Cpu::NOP, ADDR_IMMEDIATE, 0},				// $82
        {"SAX", &Cpu::SAX, ADDR_INDEXED_INDIRECT_X, 0},		// $83
        {"STY", &Cpu::STY, ADDR_ZERO_PAGE, 0},				// $84
        {"STA", &Cpu::STA, ADDR_ZERO_PAGE, 0},				// $85
        {"STX", &Cpu::STX, ADDR_ZERO_PAGE, 0},				// $86
        {"SAX", &Cpu::SAX, ADDR_ZERO_PAGE, 0},				// $87
        {"DEY", &Cpu::DEY, ADDR_IMPLICIT, 0},				// $88
        {"NOP", &Cpu::NOP, ADDR_IMMEDIATE, 0},				// $89
        {"TXA", &Cpu::TXA, ADDR_IMPLICIT, 0},				// $8A
        {"XAA", &Cpu::XAA, ADDR_IMMEDIATE, 0},				// $8B
        {"STY", &Cpu::STY, ADDR_ABSOLUTE, 0},				// $8C
        {"STA", &Cpu::STA, ADDR_ABSOLUTE, 0},				// $8D
        {"STX", &Cpu::STX, ADDR_ABSOLUTE, 0},				// $8E
        {"SAX", &Cpu::SAX, ADDR_ABSOLUTE, 0},				// $8F
        {"BCC", &Cpu::BCC, ADDR_RELATIVE, 0},				// $90
        {"STA", &Cpu::STA, ADDR_INDEXED_INDIRECT_Y, 6},		// $91
        {"STP", &Cpu::STP, ADDR_IMPLICIT, 0},				// $92
        {"AHX", &Cpu::AHX, ADDR_INDEXED_INDIRECT_Y, 6},		// $93
        {"STY", &Cpu::STY, ADDR_ZERO_PAGE_INDEXED_X, 0},	// $94
        {"STA", &Cpu::STA, ADDR_ZERO_PAGE_INDEXED_X, 0},	// $95
        {"STX", &Cpu::STX, ADDR_ZERO_PAGE_INDEXED_Y, 0},	// $96
        {"SAX", &Cpu::SAX, ADDR_ZERO_PAGE_INDEXED_Y, 0},	// $97
        {"TYA", &Cpu::TYA, ADDR_IMPLICIT, 0},				// $98
        {"STA", &Cpu::STA, ADDR_ABSOLUTE_INDEXED_Y, 5},		// $99
        {"TXS", &Cpu::TXS, ADDR_IMPLICIT, 0},				// $9A
        {"TAS", &Cpu::TAS, ADDR_ABSOLUTE_INDEXED_Y, 0},		// $9B
        {"SHY", &Cpu::SHY, ADDR_ABSOLUTE_INDEXED_X, 0},		// $9C
        {"STA", &Cpu::STA, ADDR_ABSOLUTE_INDEXED_X, 5},		// $9D
        {"SHX", &Cpu::SHX, ADDR_ABSOLUTE_INDEXED_Y, 0},		// $9E
        {"AHX", &Cpu::AHX, ADDR_ABSOLUTE_INDEXED_Y, 5},		// $9F
        {"LDY", &Cpu::LDY, ADDR_IMMEDIATE, 0},				// $A0
        {"LDA", &Cpu::LDA, ADDR_INDEXED_INDIRECT_X, 0},		// $A1
        {"LDX", &Cpu::LDX, ADDR_IMMEDIATE, 0},				// $A2
        {"LAX", &Cpu::LAX, ADDR_INDEXED_INDIRECT_X, 0},		// $A3
        {"LDY", &Cpu::LDY, ADDR_ZERO_PAGE, 0},				// $A4
        {"LDA", &Cpu::LDA, ADDR_ZERO_PAGE, 0},				// $A5
        {"LDX", &Cpu::LDX, ADDR_ZERO_PAGE, 0},				// $A6
        {"LAX", &Cpu::LAX, ADDR_ZERO_PAGE, 0},				// $A7
        {"TAY", &Cpu::TAY, ADDR_IMPLICIT, 0},				// $A8
        {"LDA", &Cpu::LDA, ADDR_IMMEDIATE, 0},				// $A9
        {"TAX", &Cpu::TAX, ADDR_IMPLICIT, 0},				// $AA
        {"LAX", &Cpu::LAX, ADDR_IMMEDIATE, 0},				// $AB
        {"LDY", &Cpu::LDY, ADDR_ABSOLUTE, 0},				// $AC
        {"LDA", &Cpu::LDA, ADDR_ABSOLUTE, 0},				// $AD
        {"LDX", &Cpu::LDX, ADDR_ABSOLUTE, 0},				// $AE
        {"LAX", &Cpu::LAX, ADDR_ABSOLUTE, 0},				// $AF
        {"BCS", &Cpu::BCS, ADDR_RELATIVE, 0},				// $B0
        {"LDA", &Cpu::LDA, ADDR_INDEXED_INDIRECT_Y, 0},		// $B1
        {"STP", &Cpu::STP, ADDR_IMPLICIT, 0},				// $B2
        {"LAX", &Cpu::LAX, ADDR_INDEXED_INDIRECT_Y, 0},		// $B3
        {"LDY", &Cpu::LDY, ADDR_ZERO_PAGE_INDEXED_X, 0},	// $B4
        {"LDA", &Cpu::LDA, ADDR_ZERO_PAGE_INDEXED_X, 0},	// $B5
        {"LDX", &Cpu::LDX, ADDR_ZERO_PAGE_INDEXED_Y, 0},	// $B6
        {"LAX", &Cpu::LAX, ADDR_ZERO_PAGE_INDEXED_Y, 0},	// $B7
        {"CLV", &Cpu::CLV, ADDR_IMPLICIT, 0},				// $B8
        {"LDA", &Cpu::LDA, ADDR_ABSOLUTE_INDEXED_Y, 0},		// $B9
        {"TSX", &Cpu::TSX, ADDR_IMPLICIT, 0},				// $BA
        {"LAS", &Cpu::LAS, ADDR_ABSOLUTE_INDEXED_Y, 0},		// $BB
        {"LDY", &Cpu::LDY, ADDR_ABSOLUTE_INDEXED_X, 0},		// $BC
        {"LDA", &Cpu::LDA, ADDR_ABSOLUTE_INDEXED_X, 0},		// $BD
        {"LDX", &Cpu::LDX, ADDR_ABSOLUTE_INDEXED_Y, 0},		// $BE
        {"LAX", &Cpu::LAX, ADDR_ABSOLUTE_INDEXED_Y, 0},		// $BF
        {"CPY", &Cpu::CPY, ADDR_IMMEDIATE, 0},				// $C0
        {"CMP", &Cpu::CMP, ADDR_INDEXED_INDIRECT_X, 0},		// $C1
        {"NOP", &Cpu::NOP, ADDR_IMMEDIATE, 0},				// $C2
        {"DCP", &Cpu::DCP, ADDR_INDEXED_INDIRECT_X, 8},		// $C3
        {"CPY", &Cpu::CPY, ADDR_ZERO_PAGE, 0},				// $C4
        {"CMP", &Cpu::CMP, ADDR_ZERO_PAGE, 0},				// $C5
        {"DEC", &Cpu::DEC, ADDR_ZERO_PAGE, 5},				// $C6
        {"DCP", &Cpu::DCP, ADDR_ZERO_PAGE, 5},				// $C7
        {"INY", &Cpu::INY, ADDR_IMPLICIT, 0},				// $C8
        {"CMP", &Cpu::CMP, ADDR_IMMEDIATE, 0},				// $C9
        {"DEX", &Cpu::DEX, ADDR_IMPLICIT, 0},				// $CA
        {"AXS", &Cpu::AXS, ADDR_IMMEDIATE, 0},				// $CB
        {"CPY", &Cpu::CPY, ADDR_ABSOLUTE, 0},				// $CC
        {"CMP", &Cpu::CMP, ADDR_ABSOLUTE, 0},				// $CD
        {"DEC", &Cpu::DEC, ADDR_ABSOLUTE, 6},				// $CE
        {"DCP", &Cpu::DCP, ADDR_ABSOLUTE, 6},				// $CF
        {"BNE", &Cpu::BNE, ADDR_RELATIVE, 0},				// $D0
        {"CMP", &Cpu::CMP, ADDR_INDEXED_INDIRECT_Y, 0},		// $D1
        {"STP", &Cpu::STP, ADDR_IMPLICIT, 0},				// $D2
        {"DCP", &Cpu::DCP, ADDR_INDEXED_INDIRECT_Y, 8},		// $D3
        {"NOP", &Cpu::NOP, ADDR_ZERO_PAGE_INDEXED_X, 0},	// $D4
        {"CMP", &Cpu::CMP, ADDR_ZERO_PAGE_INDEXED_X, 0},	// $D5
        {"DEC", &Cpu::DEC, ADDR_ZERO_PAGE_INDEXED_X, 6},	// $D6
        {"DCP", &Cpu::DCP, ADDR_ZERO_PAGE_INDEXED_X, 6},	// $D7
        {"CLD", &Cpu::CLD, ADDR_IMPLICIT, 0},				// $D8
        {"CMP", &Cpu::CMP, ADDR_ABSOLUTE_INDEXED_Y, 0},		// $D9
        {"NOP", &Cpu::NOP, ADDR_IMPLICIT, 0},				// $DA
        {"DCP", &Cpu::DCP, ADDR_ABSOLUTE_INDEXED_Y, 7},		// $DB
        {"NOP", &Cpu::NOP, ADDR_ABSOLUTE_INDEXED_X, 0},		// $DC
        {"CMP", &Cpu::CMP, ADDR_ABSOLUTE_INDEXED_X, 0},		// $DD
        {"DEC", &Cpu::DEC, ADDR_ABSOLUTE_INDEXED_X, 7},		// $DE
        {"DCP", &Cpu::DCP, ADDR_ABSOLUTE_INDEXED_X, 7},		// $DF
        {"CPX", &Cpu::CPX, ADDR_IMMEDIATE, 0},				// $E0
        {"SBC", &Cpu::SBC, ADDR_INDEXED_INDIRECT_X, 0},		// $E1
        {"NOP", &Cpu::NOP, ADDR_IMMEDIATE, 0},				// $E2
        {"ISC", &Cpu::ISC, ADDR_INDEXED_INDIRECT_X, 8},		// $E3
        {"CPX", &Cpu::CPX, ADDR_ZERO_PAGE, 0},				// $E4
        {"SBC", &Cpu::SBC, ADDR_ZERO_PAGE, 0},				// $E5
        {"INC", &Cpu::INC, ADDR_ZERO_PAGE, 5},				// $E6
        {"ISC", &Cpu::ISC, ADDR_ZERO_PAGE, 5},				// $E7
        {"INX", &Cpu::INX, ADDR_IMPLICIT, 0},				// $E8
        {"SBC", &Cpu::SBC, ADDR_IMMEDIATE, 0},				// $E9
        {"NOP", &Cpu::NOP, ADDR_IMPLICIT, 0},				// $EA
        {"SBC", &Cpu::SBC, ADDR_IMMEDIATE, 0},				// $EB
        {"CPX", &Cpu::CPX, ADDR_ABSOLUTE, 0},				// $EC
        {"SBC", &Cpu::SBC, ADDR_ABSOLUTE, 0},				// $ED
        {"INC", &Cpu::INC, ADDR_ABSOLUTE, 6},				// $EE
        {"ISC", &Cpu::ISC, ADDR_ABSOLUTE, 6},				// $EF
        {"BEQ", &Cpu::BEQ, ADDR_RELATIVE, 0},				// $F0
        {"SBC", &Cpu::SBC, ADDR_INDEXED_INDIRECT_Y, 0},		// $F1
        {"STP", &Cpu::STP, ADDR_IMPLICIT, 0},				// $F2
        {"ISC", &Cpu::ISC, ADDR_INDEXED_INDIRECT_Y, 8},		// $F3
        {"NOP", &Cpu::NOP, ADDR_ZERO_PAGE_INDEXED_X, 0},	// $F4
        {"SBC", &Cpu::SBC, ADDR_ZERO_PAGE_INDEXED_X, 0},	// $F5
        {"INC", &Cpu::INC, ADDR_ZERO_PAGE_INDEXED_X, 6},	// $F6
        {"ISC", &Cpu::ISC, ADDR_ZERO_PAGE_INDEXED_X, 6},	// $F7
        {"SED", &Cpu::SED, ADDR_IMPLICIT, 0},				// $F8
        {"SBC", &Cpu::SBC, ADDR_ABSOLUTE_INDEXED_Y, 0},		// $F9
        {"NOP", &Cpu::NOP, ADDR_IMPLICIT, 0},				// $FA
        {"ISC", &Cpu::ISC, ADDR_ABSOLUTE_INDEXED_Y, 7},		// $FB
        {"NOP", &Cpu::NOP, ADDR_ABSOLUTE_INDEXED_X, 0},		// $FC
        {"SBC", &Cpu::SBC, ADDR_ABSOLUTE_INDEXED_X, 0},		// $FD
        {"INC", &Cpu::INC, ADDR_ABSOLUTE_INDEXED_X, 7},		// $FE
        {"ISC", &Cpu::ISC, ADDR_ABSOLUTE_INDEXED_X, 7}		// $FF
};
