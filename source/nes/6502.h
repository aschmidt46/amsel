#pragma once
#include <cstdint>
#include <utility>
#include <limits>
#include <fstream>
#include "mapper.h"
#include <assert.h>
#include <iostream>
#include <memory>
#include "framework/global.h"


enum Statusbit{
    STATUS_CARRY = 0,
    STATUS_ZERO = 1,
    STATUS_INTERRUPT_DISABLE = 2,
    STATUS_DECIMAL = 3,
    STATUS_OVERFLOW = 6,
    STATUS_NEGATIVE = 7,
    STATUS_B_FLAG = 4
};

enum AddressMode{
    ADDR_IMPLICIT,
    ADDR_ACCUMULATOR,
    ADDR_IMMEDIATE,
    ADDR_ZERO_PAGE,
    ADDR_ABSOLUTE,
    ADDR_RELATIVE,
    ADDR_INDIRECT,
    ADDR_ZERO_PAGE_INDEXED_X,
    ADDR_ZERO_PAGE_INDEXED_Y,
    ADDR_ABSOLUTE_INDEXED_X,
    ADDR_ABSOLUTE_INDEXED_Y,
    ADDR_INDEXED_INDIRECT_X,
    ADDR_INDEXED_INDIRECT_Y //bzw. indirect indexed
};

class Mapper;
class Cpu{

    public:

    uint8_t internalMemory[0x0800]; // 2048 B

    uint16_t PC = 0; //Program Counter
    uint8_t A = 0; //Accumulator

    uint8_t X = 0; //Indexes
    uint8_t Y = 0;

    uint8_t SP = 0; //Stack pointer

    uint8_t P = 0; //Status register, 1 bit unbenutzt

    int remainingCycles = 0;

    size_t totalCycles = 7; //testweise

    std::shared_ptr<Mapper> mapper;

    // (Setzen, Wert)
    std::pair<bool, bool> setInterruptNextInstruction = {false, false};

    // std::ofstream log;

    Cpu(){
        for(int i = 0; i < 0x0800; i++){
            internalMemory[i] = 0;
        }
    };
    ~Cpu(){
        // log.close();
    };

    void init(int pc, std::shared_ptr<Mapper> m){
        PC = pc;
        mapper = m;
        SP = 0x00;
        P = 0b00100100; // Interrupt und Anderes Bit
        // log = std::ofstream("cout.txt");
        // assert(log.is_open());
        A = 0;
        X = 0;
        Y = 0;
        RESET();
    };

    void RESET();

    void IRQ(){
        uint16_t PCH = (PC >> 8) & 0b0000000011111111;
        pushStack(PCH);
        pushStack(PC);
        pushStack(P & 0b11101111); // B clear!
        uint8_t lower = read((uint8_t*)0xFFFE);
        uint8_t higher = read((uint8_t*)0xFFFF);
        uint16_t addr = ((uint16_t)higher << 8) | (uint16_t)lower;
        PC = addr;
        setStatus(STATUS_INTERRUPT_DISABLE, true);
    };

    void NMI(){
        uint16_t PCH = (PC >> 8) & 0b0000000011111111;
        pushStack(PCH);
        pushStack(PC);
        pushStack(P & 0b11101111); // B clear!
        uint8_t lower = read((uint8_t*)0xFFFA);
        uint8_t higher = read((uint8_t*)0xFFFB);
        uint16_t addr = ((uint16_t)higher << 8) | (uint16_t)lower;
        PC = addr;
        setStatus(STATUS_INTERRUPT_DISABLE, true);
    };

    bool IRQgenerated = false;
    bool NMIgenerated = false;
    bool NMIWasHigh = true;

    void pullNMI(){
        if(NMIWasHigh){
            NMIgenerated = true;
            NMIWasHigh = false;
        }
    };
    void pullIRQ(){
        IRQgenerated = true;;
    };

    void setStatus(Statusbit s, bool v);
    bool getStatus(Statusbit s);
    uint8_t* getMemoryAddress(AddressMode mode, uint8_t &cycles);
    uint8_t executeNextInstruction(); //Gibt verstrichene Zyklen zurück
    bool clockCPU(); // Wurde in diesem Zyklus eine Instruktion ausgeführt?
    uint8_t pollInterrupts();
    
    // suspend
    void waitFor(uint8_t cycles);

    uint8_t read(uint8_t* p);
    void write(uint8_t* p, uint8_t v);

    void pushStack(uint8_t value);
    uint8_t pullStack();

    // Debugger
    std::pair<std::string, std::vector<int>> getPrev10Instructions(); // Zeilen mit Opcode + Operanden-Länge
    std::pair<std::string, std::vector<int>> getNextNInstructions(int n);
    void incrementCircular();
    std::pair<std::string, int> formatInstruction(AddressMode m, uint8_t op1, uint8_t op2, uint16_t pc);
    std::vector<int> circular{std::vector<int>(10,-1)};
    unsigned int circularIndex = 0;

    void unimplemented(const std::string& instruction);
    void falseImplementation(const std::string& instruction);
    

    // Instruktionen
    uint8_t ADC(uint8_t* mem);
    uint8_t AND(uint8_t* mem);
    uint8_t ASL(uint8_t* mem); // mem kann A sein
    uint8_t BCC(uint8_t* mem); // signed
    uint8_t BCS(uint8_t* mem); // signed
    uint8_t BEQ(uint8_t* mem); // signed
    uint8_t BIT(uint8_t* mem);
    uint8_t BMI(uint8_t* mem); // signed
    uint8_t BNE(uint8_t* mem); // signed
    uint8_t BPL(uint8_t* mem); // signed
    uint8_t BRK(uint8_t* mem); // mem wird nicht benutzt (0)
    uint8_t BVC(uint8_t* mem); // signed
    uint8_t BVS(uint8_t* mem); // signed
    uint8_t CLC(uint8_t* mem); // mem wird nicht benutzt (0)
    uint8_t CLD(uint8_t* mem); // mem wird nicht benutzt (0)
    uint8_t CLI(uint8_t* mem); // mem wird nicht benutzt (0)
    uint8_t CLV(uint8_t* mem); // mem wird nicht benutzt (0)
    uint8_t CMP(uint8_t* mem);
    uint8_t CPX(uint8_t* mem);
    uint8_t CPY(uint8_t* mem);
    uint8_t DEC(uint8_t* mem);
    uint8_t DEX(uint8_t* mem);
    uint8_t DEY(uint8_t* mem);
    uint8_t EOR(uint8_t* mem);
    uint8_t INC(uint8_t* mem);
    uint8_t INX(uint8_t* mem); // mem wird nicht benutzt (0)
    uint8_t INY(uint8_t* mem); // mem wird nicht benutzt (0)
    uint8_t JMP(uint8_t* mem);
    uint8_t JSR(uint8_t* mem);
    uint8_t LDA(uint8_t* mem);
    uint8_t LDX(uint8_t* mem);
    uint8_t LDY(uint8_t* mem);
    uint8_t LSR(uint8_t* mem); // mem kann A sein
    uint8_t NOP(uint8_t* mem); // mem wird nicht benutzt (0)
    uint8_t ORA(uint8_t* mem);
    uint8_t PHA(uint8_t* mem); // mem wird nicht benutzt (0)
    uint8_t PHP(uint8_t* mem); // mem wird nicht benutzt (0)
    uint8_t PLA(uint8_t* mem); // mem wird nicht benutzt (0)
    uint8_t PLP(uint8_t* mem); // mem wird nicht benutzt (0)
    uint8_t ROL(uint8_t* mem); // mem kann A sein
    uint8_t ROR(uint8_t* mem); // mem kann A sein
    uint8_t RTI(uint8_t* mem); // mem wird nicht benutzt (0)
    uint8_t RTS(uint8_t* mem); // mem wird nicht benutzt (0)
    uint8_t SBC(uint8_t* mem);
    uint8_t SEC(uint8_t* mem); // mem wird nicht benutzt (0)
    uint8_t SED(uint8_t* mem); // mem wird nicht benutzt (0)
    uint8_t SEI(uint8_t* mem); // mem wird nicht benutzt (0)
    uint8_t STA(uint8_t* mem);
    uint8_t STX(uint8_t* mem);
    uint8_t STY(uint8_t* mem);
    uint8_t TAX(uint8_t* mem); // mem wird nicht benutzt (0)
    uint8_t TAY(uint8_t* mem); // mem wird nicht benutzt (0)
    uint8_t TSX(uint8_t* mem); // mem wird nicht benutzt (0)
    uint8_t TXA(uint8_t* mem); // mem wird nicht benutzt (0)
    uint8_t TXS(uint8_t* mem); // mem wird nicht benutzt (0)
    uint8_t TYA(uint8_t* mem); // mem wird nicht benutzt (0)

    // Nicht-Offizielle Instruktionen? Nicht sicher, ob das für alle zutrifft
    uint8_t STP(uint8_t* mem);
    uint8_t SLO(uint8_t* mem);
    uint8_t ANC(uint8_t* mem); // aka AAC
    uint8_t RLA(uint8_t* mem);
    uint8_t SRE(uint8_t* mem);
    uint8_t ALR(uint8_t* mem); // aka ASR
    uint8_t RRA(uint8_t* mem);
    uint8_t ARR(uint8_t* mem);
    uint8_t SAX(uint8_t* mem);
    uint8_t XAA(uint8_t* mem);
    uint8_t AHX(uint8_t* mem); // aka AXA, SHA
    uint8_t TAS(uint8_t* mem);
    uint8_t SHY(uint8_t* mem); // aka SYA
    uint8_t SHX(uint8_t* mem); // aka SXA
    uint8_t LAX(uint8_t* mem);
    uint8_t LAS(uint8_t* mem);
    uint8_t DCP(uint8_t* mem);
    uint8_t AXS(uint8_t* mem);
    uint8_t ISC(uint8_t* mem);

    struct opcode_info{
        std::string name;
        uint8_t (Cpu::*instruction) (uint8_t* mem);
        AddressMode mode;
        uint8_t cycles;
    };

    // Alle Opcodes, Quelle: https://www.nesdev.org/wiki/CPU_unofficial_opcodes
    static const opcode_info opcodes[256];
};

