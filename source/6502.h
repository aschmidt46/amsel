#pragma once
#include <cstdint>
#include <utility>
#include <limits>
#include <fstream>
#include "mapper.h"
#include <assert.h>
#include <iostream>
#include <memory>


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

    struct opcode_info{
        std::string name;
        uint8_t (Cpu::*instruction) (uint8_t* mem);
        AddressMode mode;
        uint8_t cycles;
    };

    public:

    uint8_t* internalMemory; // 2048 B

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

    std::ofstream log;

    Cpu(){
        internalMemory = new uint8_t[0x0800];
        for(int i = 0; i < 0x0800; i++){
            internalMemory[i] = 0;
        }
    };
    ~Cpu(){
        delete[] internalMemory;
        log.close();
    };

    void init(int pc, std::shared_ptr<Mapper> m){
        PC = pc;
        mapper = m;
        SP = 0x00;
        P = 0b00100100; // Interrupt und Anderes Bit
        log = std::ofstream("cout.txt");
        assert(log.is_open());
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
        pushStack(P | 0b00010000); // B gesetzt
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
        pushStack(P | 0b00010000); // B gesetzt
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
    void clockCPU();
    uint8_t pollInterrupts();
    
    // suspend
    void waitFor(uint8_t cycles);

    uint8_t read(uint8_t* p);
    void write(uint8_t* p, uint8_t v);

    void pushStack(uint8_t value);
    uint8_t pullStack();

    // Disassembly
    std::string getPrevNInstructions(int n);
    std::string getNextNInstructions(int n);
    std::pair<std::string, int> formatInstruction(AddressMode m, const std::vector<uint8_t> &operands, uint16_t pc);

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
    uint8_t ANC(uint8_t* mem);
    uint8_t RLA(uint8_t* mem);
    uint8_t SRE(uint8_t* mem);
    uint8_t ALR(uint8_t* mem);
    uint8_t RRA(uint8_t* mem);
    uint8_t ARR(uint8_t* mem);
    uint8_t SAX(uint8_t* mem);
    uint8_t XAA(uint8_t* mem);
    uint8_t AHX(uint8_t* mem);
    uint8_t TAS(uint8_t* mem);
    uint8_t SHY(uint8_t* mem);
    uint8_t SHX(uint8_t* mem);
    uint8_t LAX(uint8_t* mem);
    uint8_t LAS(uint8_t* mem);
    uint8_t DCP(uint8_t* mem);
    uint8_t AXS(uint8_t* mem);
    uint8_t ISC(uint8_t* mem);

    // Alle Opcodes, Quelle: https://www.nesdev.org/wiki/CPU_unofficial_opcodes
    const opcode_info opcodes[256] = {
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
        {"NOP", &Cpu::NOP, ADDR_IMPLICIT, 0},				// $1A
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
        {"AHX", &Cpu::AHX, ADDR_INDEXED_INDIRECT_Y, 0},		// $93
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
        {"AHX", &Cpu::AHX, ADDR_ABSOLUTE_INDEXED_Y, 0},		// $9F
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
};

