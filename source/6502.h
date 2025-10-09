#pragma once
#include <cstdint>
#include <utility>
#include <limits>
#include <fstream>
#include "mapper.h"
#include <assert.h>


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
        uint8_t (Cpu::*instruction) (uint8_t* mem);
        AddressMode mode;
        uint8_t cycles;
    };

    public:

    uint8_t* internalMemory; // 2048 B

    uint16_t PC; //Program Counter
    uint8_t A; //Accumulator

    uint8_t X; //Indexes
    uint8_t Y;

    uint8_t SP; //Stack pointer

    uint8_t P; //Status register, 1 bit unbenutzt

    int remainingCycles;

    size_t totalCycles = 7; //testweise

    Mapper* mapper;

    // (Setzen, Wert)
    std::pair<bool, bool> setInterruptNextInstruction = {false, false};

    std::ofstream log;

    Cpu(){
        internalMemory = new uint8_t[0x0800];
    };
    ~Cpu(){
        delete[] internalMemory;
        log.close();
    };

    void init(int pc, Mapper* m){
        PC = pc;
        mapper = m;
        SP = 0xFD;
        P = 0b00100100; // Interrupt und Anderes Bit
        log = std::ofstream("cout.txt");
        assert(log.is_open());
        A = 0;
        X = 0;
        Y = 0;
    };

    void setStatus(Statusbit s, bool v);
    bool getStatus(Statusbit s);
    uint8_t* getMemoryAddress(AddressMode mode, uint8_t &cycles);
    uint8_t executeNextInstruction(); //Gibt verstrichene Zyklen zurück
    void clockCPU();
    void waitFor(uint8_t cycles);

    uint8_t read(uint8_t* p);
    void write(uint8_t* p, uint8_t v);

    void pushStack(uint8_t value);
    uint8_t pullStack();

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
        {&Cpu::BRK, ADDR_IMMEDIATE, 7},				// $00
        {&Cpu::ORA, ADDR_INDEXED_INDIRECT_X, 0},		// $01
        {&Cpu::STP, ADDR_IMPLICIT, 0},				// $02
        {&Cpu::SLO, ADDR_INDEXED_INDIRECT_X, 8},		// $03
        {&Cpu::NOP, ADDR_ZERO_PAGE, 0},				// $04
        {&Cpu::ORA, ADDR_ZERO_PAGE, 0},				// $05
        {&Cpu::ASL, ADDR_ZERO_PAGE, 5},				// $06
        {&Cpu::SLO, ADDR_ZERO_PAGE, 5},				// $07
        {&Cpu::PHP, ADDR_IMPLICIT, 3},				// $08
        {&Cpu::ORA, ADDR_IMMEDIATE, 0},				// $09
        {&Cpu::ASL, ADDR_IMPLICIT, 2},				// $0A
        {&Cpu::ANC, ADDR_IMMEDIATE, 0},				// $0B
        {&Cpu::NOP, ADDR_ABSOLUTE, 0},				// $0C
        {&Cpu::ORA, ADDR_ABSOLUTE, 0},				// $0D
        {&Cpu::ASL, ADDR_ABSOLUTE, 6},				// $0E
        {&Cpu::SLO, ADDR_ABSOLUTE, 6},				// $0F
        {&Cpu::BPL, ADDR_RELATIVE, 0},				// $10
        {&Cpu::ORA, ADDR_INDEXED_INDIRECT_Y, 0},		// $11
        {&Cpu::STP, ADDR_IMPLICIT, 0},				// $12
        {&Cpu::SLO, ADDR_INDEXED_INDIRECT_Y, 8},		// $13
        {&Cpu::NOP, ADDR_ZERO_PAGE_INDEXED_X, 0},	// $14
        {&Cpu::ORA, ADDR_ZERO_PAGE_INDEXED_X, 0},	// $15
        {&Cpu::ASL, ADDR_ZERO_PAGE_INDEXED_X, 6},	// $16
        {&Cpu::SLO, ADDR_ZERO_PAGE_INDEXED_X, 6},	// $17
        {&Cpu::CLC, ADDR_IMPLICIT, 0},				// $18
        {&Cpu::ORA, ADDR_ABSOLUTE_INDEXED_Y, 0},		// $19
        {&Cpu::NOP, ADDR_IMPLICIT, 0},				// $1A
        {&Cpu::SLO, ADDR_ABSOLUTE_INDEXED_Y, 7},		// $1B
        {&Cpu::NOP, ADDR_ABSOLUTE_INDEXED_X, 0},		// $1C
        {&Cpu::ORA, ADDR_ABSOLUTE_INDEXED_X, 0},		// $1D
        {&Cpu::ASL, ADDR_ABSOLUTE_INDEXED_X, 7},		// $1E
        {&Cpu::SLO, ADDR_ABSOLUTE_INDEXED_X, 7},		// $1F
        {&Cpu::JSR, ADDR_ABSOLUTE, 6},				// $20
        {&Cpu::AND, ADDR_INDEXED_INDIRECT_X, 0},		// $21
        {&Cpu::STP, ADDR_IMPLICIT, 0},				// $22
        {&Cpu::RLA, ADDR_INDEXED_INDIRECT_X, 8},		// $23
        {&Cpu::BIT, ADDR_ZERO_PAGE, 0},				// $24
        {&Cpu::AND, ADDR_ZERO_PAGE, 0},				// $25
        {&Cpu::ROL, ADDR_ZERO_PAGE, 5},				// $26
        {&Cpu::RLA, ADDR_ZERO_PAGE, 5},				// $27
        {&Cpu::PLP, ADDR_IMPLICIT, 4},				// $28
        {&Cpu::AND, ADDR_IMMEDIATE, 0},				// $29
        {&Cpu::ROL, ADDR_IMPLICIT, 2},				// $2A
        {&Cpu::ANC, ADDR_IMMEDIATE, 0},				// $2B
        {&Cpu::BIT, ADDR_ABSOLUTE, 0},				// $2C
        {&Cpu::AND, ADDR_ABSOLUTE, 0},				// $2D
        {&Cpu::ROL, ADDR_ABSOLUTE, 6},				// $2E
        {&Cpu::RLA, ADDR_ABSOLUTE, 6},				// $2F
        {&Cpu::BMI, ADDR_RELATIVE, 0},				// $30
        {&Cpu::AND, ADDR_INDEXED_INDIRECT_Y, 0},		// $31
        {&Cpu::STP, ADDR_IMPLICIT, 0},				// $32
        {&Cpu::RLA, ADDR_INDEXED_INDIRECT_Y, 8},		// $33
        {&Cpu::NOP, ADDR_ZERO_PAGE_INDEXED_X, 0},	// $34
        {&Cpu::AND, ADDR_ZERO_PAGE_INDEXED_X, 0},	// $35
        {&Cpu::ROL, ADDR_ZERO_PAGE_INDEXED_X, 6},	// $36
        {&Cpu::RLA, ADDR_ZERO_PAGE_INDEXED_X, 6},	// $37
        {&Cpu::SEC, ADDR_IMPLICIT, 0},				// $38
        {&Cpu::AND, ADDR_ABSOLUTE_INDEXED_Y, 0},		// $39
        {&Cpu::NOP, ADDR_IMPLICIT, 0},				// $3A
        {&Cpu::RLA, ADDR_ABSOLUTE_INDEXED_Y, 7},		// $3B
        {&Cpu::NOP, ADDR_ABSOLUTE_INDEXED_X, 0},		// $3C
        {&Cpu::AND, ADDR_ABSOLUTE_INDEXED_X, 0},		// $3D
        {&Cpu::ROL, ADDR_ABSOLUTE_INDEXED_X, 7},		// $3E
        {&Cpu::RLA, ADDR_ABSOLUTE_INDEXED_X, 7},		// $3F
        {&Cpu::RTI, ADDR_IMPLICIT, 6},				// $40
        {&Cpu::EOR, ADDR_INDEXED_INDIRECT_X, 0},		// $41
        {&Cpu::STP, ADDR_IMPLICIT, 0},				// $42
        {&Cpu::SRE, ADDR_INDEXED_INDIRECT_X, 8},		// $43
        {&Cpu::NOP, ADDR_ZERO_PAGE, 0},				// $44
        {&Cpu::EOR, ADDR_ZERO_PAGE, 0},				// $45
        {&Cpu::LSR, ADDR_ZERO_PAGE, 5},				// $46
        {&Cpu::SRE, ADDR_ZERO_PAGE, 5},				// $47
        {&Cpu::PHA, ADDR_IMPLICIT, 3},				// $48
        {&Cpu::EOR, ADDR_IMMEDIATE, 0},				// $49
        {&Cpu::LSR, ADDR_IMPLICIT, 2},				// $4A
        {&Cpu::ALR, ADDR_IMMEDIATE, 0},				// $4B
        {&Cpu::JMP, ADDR_ABSOLUTE, 3},				// $4C
        {&Cpu::EOR, ADDR_ABSOLUTE, 0},				// $4D
        {&Cpu::LSR, ADDR_ABSOLUTE, 6},				// $4E
        {&Cpu::SRE, ADDR_ABSOLUTE, 6},				// $4F
        {&Cpu::BVC, ADDR_RELATIVE, 0},				// $50
        {&Cpu::EOR, ADDR_INDEXED_INDIRECT_Y, 0},		// $51
        {&Cpu::STP, ADDR_IMPLICIT, 0},				// $52
        {&Cpu::SRE, ADDR_INDEXED_INDIRECT_Y, 8},		// $53
        {&Cpu::NOP, ADDR_ZERO_PAGE_INDEXED_X, 0},	// $54
        {&Cpu::EOR, ADDR_ZERO_PAGE_INDEXED_X, 0},	// $55
        {&Cpu::LSR, ADDR_ZERO_PAGE_INDEXED_X, 6},	// $56
        {&Cpu::SRE, ADDR_ZERO_PAGE_INDEXED_X, 6},	// $57
        {&Cpu::CLI, ADDR_IMPLICIT, 0},				// $58
        {&Cpu::EOR, ADDR_ABSOLUTE_INDEXED_Y, 0},		// $59
        {&Cpu::NOP, ADDR_IMPLICIT, 0},				// $5A
        {&Cpu::SRE, ADDR_ABSOLUTE_INDEXED_Y, 7},		// $5B
        {&Cpu::NOP, ADDR_ABSOLUTE_INDEXED_X, 0},		// $5C
        {&Cpu::EOR, ADDR_ABSOLUTE_INDEXED_X, 0},		// $5D
        {&Cpu::LSR, ADDR_ABSOLUTE_INDEXED_X, 7},		// $5E
        {&Cpu::SRE, ADDR_ABSOLUTE_INDEXED_X, 7},		// $5F
        {&Cpu::RTS, ADDR_IMPLICIT, 6},				// $60
        {&Cpu::ADC, ADDR_INDEXED_INDIRECT_X, 0},		// $61
        {&Cpu::STP, ADDR_IMPLICIT, 0},				// $62
        {&Cpu::RRA, ADDR_INDEXED_INDIRECT_X, 8},		// $63
        {&Cpu::NOP, ADDR_ZERO_PAGE, 0},				// $64
        {&Cpu::ADC, ADDR_ZERO_PAGE, 0},				// $65
        {&Cpu::ROR, ADDR_ZERO_PAGE, 5},				// $66
        {&Cpu::RRA, ADDR_ZERO_PAGE, 5},				// $67
        {&Cpu::PLA, ADDR_IMPLICIT, 4},				// $68
        {&Cpu::ADC, ADDR_IMMEDIATE, 0},				// $69
        {&Cpu::ROR, ADDR_IMPLICIT, 2},				// $6A
        {&Cpu::ARR, ADDR_IMMEDIATE, 0},				// $6B
        {&Cpu::JMP, ADDR_INDIRECT, 5},				// $6C
        {&Cpu::ADC, ADDR_ABSOLUTE, 0},				// $6D
        {&Cpu::ROR, ADDR_ABSOLUTE, 6},				// $6E
        {&Cpu::RRA, ADDR_ABSOLUTE, 6},				// $6F
        {&Cpu::BVS, ADDR_RELATIVE, 0},				// $70
        {&Cpu::ADC, ADDR_INDEXED_INDIRECT_Y, 0},		// $71
        {&Cpu::STP, ADDR_IMPLICIT, 0},				// $72
        {&Cpu::RRA, ADDR_INDEXED_INDIRECT_Y, 8},		// $73
        {&Cpu::NOP, ADDR_ZERO_PAGE_INDEXED_X, 0},	// $74
        {&Cpu::ADC, ADDR_ZERO_PAGE_INDEXED_X, 0},	// $75
        {&Cpu::ROR, ADDR_ZERO_PAGE_INDEXED_X, 6},	// $76
        {&Cpu::RRA, ADDR_ZERO_PAGE_INDEXED_X, 6},	// $77
        {&Cpu::SEI, ADDR_IMPLICIT, 0},				// $78
        {&Cpu::ADC, ADDR_ABSOLUTE_INDEXED_Y, 0},		// $79
        {&Cpu::NOP, ADDR_IMPLICIT, 0},				// $7A
        {&Cpu::RRA, ADDR_ABSOLUTE_INDEXED_Y, 7},		// $7B
        {&Cpu::NOP, ADDR_ABSOLUTE_INDEXED_X, 0},		// $7C
        {&Cpu::ADC, ADDR_ABSOLUTE_INDEXED_X, 0},		// $7D
        {&Cpu::ROR, ADDR_ABSOLUTE_INDEXED_X, 7},		// $7E
        {&Cpu::RRA, ADDR_ABSOLUTE_INDEXED_X, 7},		// $7F
        {&Cpu::NOP, ADDR_IMMEDIATE, 0},				// $80
        {&Cpu::STA, ADDR_INDEXED_INDIRECT_X, 6},		// $81
        {&Cpu::NOP, ADDR_IMMEDIATE, 0},				// $82
        {&Cpu::SAX, ADDR_INDEXED_INDIRECT_X, 0},		// $83
        {&Cpu::STY, ADDR_ZERO_PAGE, 0},				// $84
        {&Cpu::STA, ADDR_ZERO_PAGE, 0},				// $85
        {&Cpu::STX, ADDR_ZERO_PAGE, 0},				// $86
        {&Cpu::SAX, ADDR_ZERO_PAGE, 0},				// $87
        {&Cpu::DEY, ADDR_IMPLICIT, 0},				// $88
        {&Cpu::NOP, ADDR_IMMEDIATE, 0},				// $89
        {&Cpu::TXA, ADDR_IMPLICIT, 0},				// $8A
        {&Cpu::XAA, ADDR_IMMEDIATE, 0},				// $8B
        {&Cpu::STY, ADDR_ABSOLUTE, 0},				// $8C
        {&Cpu::STA, ADDR_ABSOLUTE, 0},				// $8D
        {&Cpu::STX, ADDR_ABSOLUTE, 0},				// $8E
        {&Cpu::SAX, ADDR_ABSOLUTE, 0},				// $8F
        {&Cpu::BCC, ADDR_RELATIVE, 0},				// $90
        {&Cpu::STA, ADDR_INDEXED_INDIRECT_Y, 6},		// $91
        {&Cpu::STP, ADDR_IMPLICIT, 0},				// $92
        {&Cpu::AHX, ADDR_INDEXED_INDIRECT_Y, 0},		// $93
        {&Cpu::STY, ADDR_ZERO_PAGE_INDEXED_X, 0},	// $94
        {&Cpu::STA, ADDR_ZERO_PAGE_INDEXED_X, 0},	// $95
        {&Cpu::STX, ADDR_ZERO_PAGE_INDEXED_Y, 0},	// $96
        {&Cpu::SAX, ADDR_ZERO_PAGE_INDEXED_Y, 0},	// $97
        {&Cpu::TYA, ADDR_IMPLICIT, 0},				// $98
        {&Cpu::STA, ADDR_ABSOLUTE_INDEXED_Y, 5},		// $99
        {&Cpu::TXS, ADDR_IMPLICIT, 0},				// $9A
        {&Cpu::TAS, ADDR_ABSOLUTE_INDEXED_Y, 0},		// $9B
        {&Cpu::SHY, ADDR_ABSOLUTE_INDEXED_X, 0},		// $9C
        {&Cpu::STA, ADDR_ABSOLUTE_INDEXED_X, 5},		// $9D
        {&Cpu::SHX, ADDR_ABSOLUTE_INDEXED_Y, 0},		// $9E
        {&Cpu::AHX, ADDR_ABSOLUTE_INDEXED_Y, 0},		// $9F
        {&Cpu::LDY, ADDR_IMMEDIATE, 0},				// $A0
        {&Cpu::LDA, ADDR_INDEXED_INDIRECT_X, 0},		// $A1
        {&Cpu::LDX, ADDR_IMMEDIATE, 0},				// $A2
        {&Cpu::LAX, ADDR_INDEXED_INDIRECT_X, 0},		// $A3
        {&Cpu::LDY, ADDR_ZERO_PAGE, 0},				// $A4
        {&Cpu::LDA, ADDR_ZERO_PAGE, 0},				// $A5
        {&Cpu::LDX, ADDR_ZERO_PAGE, 0},				// $A6
        {&Cpu::LAX, ADDR_ZERO_PAGE, 0},				// $A7
        {&Cpu::TAY, ADDR_IMPLICIT, 0},				// $A8
        {&Cpu::LDA, ADDR_IMMEDIATE, 0},				// $A9
        {&Cpu::TAX, ADDR_IMPLICIT, 0},				// $AA
        {&Cpu::LAX, ADDR_IMMEDIATE, 0},				// $AB
        {&Cpu::LDY, ADDR_ABSOLUTE, 0},				// $AC
        {&Cpu::LDA, ADDR_ABSOLUTE, 0},				// $AD
        {&Cpu::LDX, ADDR_ABSOLUTE, 0},				// $AE
        {&Cpu::LAX, ADDR_ABSOLUTE, 0},				// $AF
        {&Cpu::BCS, ADDR_RELATIVE, 0},				// $B0
        {&Cpu::LDA, ADDR_INDEXED_INDIRECT_Y, 0},		// $B1
        {&Cpu::STP, ADDR_IMPLICIT, 0},				// $B2
        {&Cpu::LAX, ADDR_INDEXED_INDIRECT_Y, 0},		// $B3
        {&Cpu::LDY, ADDR_ZERO_PAGE_INDEXED_X, 0},	// $B4
        {&Cpu::LDA, ADDR_ZERO_PAGE_INDEXED_X, 0},	// $B5
        {&Cpu::LDX, ADDR_ZERO_PAGE_INDEXED_Y, 0},	// $B6
        {&Cpu::LAX, ADDR_ZERO_PAGE_INDEXED_Y, 0},	// $B7
        {&Cpu::CLV, ADDR_IMPLICIT, 0},				// $B8
        {&Cpu::LDA, ADDR_ABSOLUTE_INDEXED_Y, 0},		// $B9
        {&Cpu::TSX, ADDR_IMPLICIT, 0},				// $BA
        {&Cpu::LAS, ADDR_ABSOLUTE_INDEXED_Y, 0},		// $BB
        {&Cpu::LDY, ADDR_ABSOLUTE_INDEXED_X, 0},		// $BC
        {&Cpu::LDA, ADDR_ABSOLUTE_INDEXED_X, 0},		// $BD
        {&Cpu::LDX, ADDR_ABSOLUTE_INDEXED_Y, 0},		// $BE
        {&Cpu::LAX, ADDR_ABSOLUTE_INDEXED_Y, 0},		// $BF
        {&Cpu::CPY, ADDR_IMMEDIATE, 0},				// $C0
        {&Cpu::CMP, ADDR_INDEXED_INDIRECT_X, 0},		// $C1
        {&Cpu::NOP, ADDR_IMMEDIATE, 0},				// $C2
        {&Cpu::DCP, ADDR_INDEXED_INDIRECT_X, 8},		// $C3
        {&Cpu::CPY, ADDR_ZERO_PAGE, 0},				// $C4
        {&Cpu::CMP, ADDR_ZERO_PAGE, 0},				// $C5
        {&Cpu::DEC, ADDR_ZERO_PAGE, 5},				// $C6
        {&Cpu::DCP, ADDR_ZERO_PAGE, 5},				// $C7
        {&Cpu::INY, ADDR_IMPLICIT, 0},				// $C8
        {&Cpu::CMP, ADDR_IMMEDIATE, 0},				// $C9
        {&Cpu::DEX, ADDR_IMPLICIT, 0},				// $CA
        {&Cpu::AXS, ADDR_IMMEDIATE, 0},				// $CB
        {&Cpu::CPY, ADDR_ABSOLUTE, 0},				// $CC
        {&Cpu::CMP, ADDR_ABSOLUTE, 0},				// $CD
        {&Cpu::DEC, ADDR_ABSOLUTE, 6},				// $CE
        {&Cpu::DCP, ADDR_ABSOLUTE, 6},				// $CF
        {&Cpu::BNE, ADDR_RELATIVE, 0},				// $D0
        {&Cpu::CMP, ADDR_INDEXED_INDIRECT_Y, 0},		// $D1
        {&Cpu::STP, ADDR_IMPLICIT, 0},				// $D2
        {&Cpu::DCP, ADDR_INDEXED_INDIRECT_Y, 8},		// $D3
        {&Cpu::NOP, ADDR_ZERO_PAGE_INDEXED_X, 0},	// $D4
        {&Cpu::CMP, ADDR_ZERO_PAGE_INDEXED_X, 0},	// $D5
        {&Cpu::DEC, ADDR_ZERO_PAGE_INDEXED_X, 6},	// $D6
        {&Cpu::DCP, ADDR_ZERO_PAGE_INDEXED_X, 6},	// $D7
        {&Cpu::CLD, ADDR_IMPLICIT, 0},				// $D8
        {&Cpu::CMP, ADDR_ABSOLUTE_INDEXED_Y, 0},		// $D9
        {&Cpu::NOP, ADDR_IMPLICIT, 0},				// $DA
        {&Cpu::DCP, ADDR_ABSOLUTE_INDEXED_Y, 7},		// $DB
        {&Cpu::NOP, ADDR_ABSOLUTE_INDEXED_X, 0},		// $DC
        {&Cpu::CMP, ADDR_ABSOLUTE_INDEXED_X, 0},		// $DD
        {&Cpu::DEC, ADDR_ABSOLUTE_INDEXED_X, 7},		// $DE
        {&Cpu::DCP, ADDR_ABSOLUTE_INDEXED_X, 7},		// $DF
        {&Cpu::CPX, ADDR_IMMEDIATE, 0},				// $E0
        {&Cpu::SBC, ADDR_INDEXED_INDIRECT_X, 0},		// $E1
        {&Cpu::NOP, ADDR_IMMEDIATE, 0},				// $E2
        {&Cpu::ISC, ADDR_INDEXED_INDIRECT_X, 8},		// $E3
        {&Cpu::CPX, ADDR_ZERO_PAGE, 0},				// $E4
        {&Cpu::SBC, ADDR_ZERO_PAGE, 0},				// $E5
        {&Cpu::INC, ADDR_ZERO_PAGE, 5},				// $E6
        {&Cpu::ISC, ADDR_ZERO_PAGE, 5},				// $E7
        {&Cpu::INX, ADDR_IMPLICIT, 0},				// $E8
        {&Cpu::SBC, ADDR_IMMEDIATE, 0},				// $E9
        {&Cpu::NOP, ADDR_IMPLICIT, 0},				// $EA
        {&Cpu::SBC, ADDR_IMMEDIATE, 0},				// $EB
        {&Cpu::CPX, ADDR_ABSOLUTE, 0},				// $EC
        {&Cpu::SBC, ADDR_ABSOLUTE, 0},				// $ED
        {&Cpu::INC, ADDR_ABSOLUTE, 6},				// $EE
        {&Cpu::ISC, ADDR_ABSOLUTE, 6},				// $EF
        {&Cpu::BEQ, ADDR_RELATIVE, 0},				// $F0
        {&Cpu::SBC, ADDR_INDEXED_INDIRECT_Y, 0},		// $F1
        {&Cpu::STP, ADDR_IMPLICIT, 0},				// $F2
        {&Cpu::ISC, ADDR_INDEXED_INDIRECT_Y, 8},		// $F3
        {&Cpu::NOP, ADDR_ZERO_PAGE_INDEXED_X, 0},	// $F4
        {&Cpu::SBC, ADDR_ZERO_PAGE_INDEXED_X, 0},	// $F5
        {&Cpu::INC, ADDR_ZERO_PAGE_INDEXED_X, 6},	// $F6
        {&Cpu::ISC, ADDR_ZERO_PAGE_INDEXED_X, 6},	// $F7
        {&Cpu::SED, ADDR_IMPLICIT, 0},				// $F8
        {&Cpu::SBC, ADDR_ABSOLUTE_INDEXED_Y, 0},		// $F9
        {&Cpu::NOP, ADDR_IMPLICIT, 0},				// $FA
        {&Cpu::ISC, ADDR_ABSOLUTE_INDEXED_Y, 7},		// $FB
        {&Cpu::NOP, ADDR_ABSOLUTE_INDEXED_X, 0},		// $FC
        {&Cpu::SBC, ADDR_ABSOLUTE_INDEXED_X, 0},		// $FD
        {&Cpu::INC, ADDR_ABSOLUTE_INDEXED_X, 7},		// $FE
        {&Cpu::ISC, ADDR_ABSOLUTE_INDEXED_X, 7}		// $FF
    };
};

