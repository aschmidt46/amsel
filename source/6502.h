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
        log = std::ofstream("cout.txt", std::ios::out);
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
        {&BRK, ADDR_IMMEDIATE, 7},				// $00
        {&ORA, ADDR_INDEXED_INDIRECT_X, 0},		// $01
        {&STP, ADDR_IMPLICIT, 0},				// $02
        {&SLO, ADDR_INDEXED_INDIRECT_X, 8},		// $03
        {&NOP, ADDR_ZERO_PAGE, 0},				// $04
        {&ORA, ADDR_ZERO_PAGE, 0},				// $05
        {&ASL, ADDR_ZERO_PAGE, 5},				// $06
        {&SLO, ADDR_ZERO_PAGE, 5},				// $07
        {&PHP, ADDR_IMPLICIT, 3},				// $08
        {&ORA, ADDR_IMMEDIATE, 0},				// $09
        {&ASL, ADDR_IMPLICIT, 2},				// $0A
        {&ANC, ADDR_IMMEDIATE, 0},				// $0B
        {&NOP, ADDR_ABSOLUTE, 0},				// $0C
        {&ORA, ADDR_ABSOLUTE, 0},				// $0D
        {&ASL, ADDR_ABSOLUTE, 6},				// $0E
        {&SLO, ADDR_ABSOLUTE, 6},				// $0F
        {&BPL, ADDR_RELATIVE, 0},				// $10
        {&ORA, ADDR_INDEXED_INDIRECT_Y, 0},		// $11
        {&STP, ADDR_IMPLICIT, 0},				// $12
        {&SLO, ADDR_INDEXED_INDIRECT_Y, 8},		// $13
        {&NOP, ADDR_ZERO_PAGE_INDEXED_X, 0},	// $14
        {&ORA, ADDR_ZERO_PAGE_INDEXED_X, 0},	// $15
        {&ASL, ADDR_ZERO_PAGE_INDEXED_X, 6},	// $16
        {&SLO, ADDR_ZERO_PAGE_INDEXED_X, 6},	// $17
        {&CLC, ADDR_IMPLICIT, 0},				// $18
        {&ORA, ADDR_ABSOLUTE_INDEXED_Y, 0},		// $19
        {&NOP, ADDR_IMPLICIT, 0},				// $1A
        {&SLO, ADDR_ABSOLUTE_INDEXED_Y, 7},		// $1B
        {&NOP, ADDR_ABSOLUTE_INDEXED_X, 0},		// $1C
        {&ORA, ADDR_ABSOLUTE_INDEXED_X, 0},		// $1D
        {&ASL, ADDR_ABSOLUTE_INDEXED_X, 7},		// $1E
        {&SLO, ADDR_ABSOLUTE_INDEXED_X, 7},		// $1F
        {&JSR, ADDR_ABSOLUTE, 6},				// $20
        {&AND, ADDR_INDEXED_INDIRECT_X, 0},		// $21
        {&STP, ADDR_IMPLICIT, 0},				// $22
        {&RLA, ADDR_INDEXED_INDIRECT_X, 8},		// $23
        {&BIT, ADDR_ZERO_PAGE, 0},				// $24
        {&AND, ADDR_ZERO_PAGE, 0},				// $25
        {&ROL, ADDR_ZERO_PAGE, 5},				// $26
        {&RLA, ADDR_ZERO_PAGE, 5},				// $27
        {&PLP, ADDR_IMPLICIT, 4},				// $28
        {&AND, ADDR_IMMEDIATE, 0},				// $29
        {&ROL, ADDR_IMPLICIT, 2},				// $2A
        {&ANC, ADDR_IMMEDIATE, 0},				// $2B
        {&BIT, ADDR_ABSOLUTE, 0},				// $2C
        {&AND, ADDR_ABSOLUTE, 0},				// $2D
        {&ROL, ADDR_ABSOLUTE, 6},				// $2E
        {&RLA, ADDR_ABSOLUTE, 6},				// $2F
        {&BMI, ADDR_RELATIVE, 0},				// $30
        {&AND, ADDR_INDEXED_INDIRECT_Y, 0},		// $31
        {&STP, ADDR_IMPLICIT, 0},				// $32
        {&RLA, ADDR_INDEXED_INDIRECT_Y, 8},		// $33
        {&NOP, ADDR_ZERO_PAGE_INDEXED_X, 0},	// $34
        {&AND, ADDR_ZERO_PAGE_INDEXED_X, 0},	// $35
        {&ROL, ADDR_ZERO_PAGE_INDEXED_X, 6},	// $36
        {&RLA, ADDR_ZERO_PAGE_INDEXED_X, 6},	// $37
        {&SEC, ADDR_IMPLICIT, 0},				// $38
        {&AND, ADDR_ABSOLUTE_INDEXED_Y, 0},		// $39
        {&NOP, ADDR_IMPLICIT, 0},				// $3A
        {&RLA, ADDR_ABSOLUTE_INDEXED_Y, 7},		// $3B
        {&NOP, ADDR_ABSOLUTE_INDEXED_X, 0},		// $3C
        {&AND, ADDR_ABSOLUTE_INDEXED_X, 0},		// $3D
        {&ROL, ADDR_ABSOLUTE_INDEXED_X, 7},		// $3E
        {&RLA, ADDR_ABSOLUTE_INDEXED_X, 7},		// $3F
        {&RTI, ADDR_IMPLICIT, 6},				// $40
        {&EOR, ADDR_INDEXED_INDIRECT_X, 0},		// $41
        {&STP, ADDR_IMPLICIT, 0},				// $42
        {&SRE, ADDR_INDEXED_INDIRECT_X, 8},		// $43
        {&NOP, ADDR_ZERO_PAGE, 0},				// $44
        {&EOR, ADDR_ZERO_PAGE, 0},				// $45
        {&LSR, ADDR_ZERO_PAGE, 5},				// $46
        {&SRE, ADDR_ZERO_PAGE, 5},				// $47
        {&PHA, ADDR_IMPLICIT, 3},				// $48
        {&EOR, ADDR_IMMEDIATE, 0},				// $49
        {&LSR, ADDR_IMPLICIT, 2},				// $4A
        {&ALR, ADDR_IMMEDIATE, 0},				// $4B
        {&JMP, ADDR_ABSOLUTE, 3},				// $4C
        {&EOR, ADDR_ABSOLUTE, 0},				// $4D
        {&LSR, ADDR_ABSOLUTE, 6},				// $4E
        {&SRE, ADDR_ABSOLUTE, 6},				// $4F
        {&BVC, ADDR_RELATIVE, 0},				// $50
        {&EOR, ADDR_INDEXED_INDIRECT_Y, 0},		// $51
        {&STP, ADDR_IMPLICIT, 0},				// $52
        {&SRE, ADDR_INDEXED_INDIRECT_Y, 8},		// $53
        {&NOP, ADDR_ZERO_PAGE_INDEXED_X, 0},	// $54
        {&EOR, ADDR_ZERO_PAGE_INDEXED_X, 0},	// $55
        {&LSR, ADDR_ZERO_PAGE_INDEXED_X, 6},	// $56
        {&SRE, ADDR_ZERO_PAGE_INDEXED_X, 6},	// $57
        {&CLI, ADDR_IMPLICIT, 0},				// $58
        {&EOR, ADDR_ABSOLUTE_INDEXED_Y, 0},		// $59
        {&NOP, ADDR_IMPLICIT, 0},				// $5A
        {&SRE, ADDR_ABSOLUTE_INDEXED_Y, 7},		// $5B
        {&NOP, ADDR_ABSOLUTE_INDEXED_X, 0},		// $5C
        {&EOR, ADDR_ABSOLUTE_INDEXED_X, 0},		// $5D
        {&LSR, ADDR_ABSOLUTE_INDEXED_X, 7},		// $5E
        {&SRE, ADDR_ABSOLUTE_INDEXED_X, 7},		// $5F
        {&RTS, ADDR_IMPLICIT, 6},				// $60
        {&ADC, ADDR_INDEXED_INDIRECT_X, 0},		// $61
        {&STP, ADDR_IMPLICIT, 0},				// $62
        {&RRA, ADDR_INDEXED_INDIRECT_X, 8},		// $63
        {&NOP, ADDR_ZERO_PAGE, 0},				// $64
        {&ADC, ADDR_ZERO_PAGE, 0},				// $65
        {&ROR, ADDR_ZERO_PAGE, 5},				// $66
        {&RRA, ADDR_ZERO_PAGE, 5},				// $67
        {&PLA, ADDR_IMPLICIT, 4},				// $68
        {&ADC, ADDR_IMMEDIATE, 0},				// $69
        {&ROR, ADDR_IMPLICIT, 2},				// $6A
        {&ARR, ADDR_IMMEDIATE, 0},				// $6B
        {&JMP, ADDR_INDIRECT, 5},				// $6C
        {&ADC, ADDR_ABSOLUTE, 0},				// $6D
        {&ROR, ADDR_ABSOLUTE, 6},				// $6E
        {&RRA, ADDR_ABSOLUTE, 6},				// $6F
        {&BVS, ADDR_RELATIVE, 0},				// $70
        {&ADC, ADDR_INDEXED_INDIRECT_Y, 0},		// $71
        {&STP, ADDR_IMPLICIT, 0},				// $72
        {&RRA, ADDR_INDEXED_INDIRECT_Y, 8},		// $73
        {&NOP, ADDR_ZERO_PAGE_INDEXED_X, 0},	// $74
        {&ADC, ADDR_ZERO_PAGE_INDEXED_X, 0},	// $75
        {&ROR, ADDR_ZERO_PAGE_INDEXED_X, 6},	// $76
        {&RRA, ADDR_ZERO_PAGE_INDEXED_X, 6},	// $77
        {&SEI, ADDR_IMPLICIT, 0},				// $78
        {&ADC, ADDR_ABSOLUTE_INDEXED_Y, 0},		// $79
        {&NOP, ADDR_IMPLICIT, 0},				// $7A
        {&RRA, ADDR_ABSOLUTE_INDEXED_Y, 7},		// $7B
        {&NOP, ADDR_ABSOLUTE_INDEXED_X, 0},		// $7C
        {&ADC, ADDR_ABSOLUTE_INDEXED_X, 0},		// $7D
        {&ROR, ADDR_ABSOLUTE_INDEXED_X, 7},		// $7E
        {&RRA, ADDR_ABSOLUTE_INDEXED_X, 7},		// $7F
        {&NOP, ADDR_IMMEDIATE, 0},				// $80
        {&STA, ADDR_INDEXED_INDIRECT_X, 6},		// $81
        {&NOP, ADDR_IMMEDIATE, 0},				// $82
        {&SAX, ADDR_INDEXED_INDIRECT_X, 0},		// $83
        {&STY, ADDR_ZERO_PAGE, 0},				// $84
        {&STA, ADDR_ZERO_PAGE, 0},				// $85
        {&STX, ADDR_ZERO_PAGE, 0},				// $86
        {&SAX, ADDR_ZERO_PAGE, 0},				// $87
        {&DEY, ADDR_IMPLICIT, 0},				// $88
        {&NOP, ADDR_IMMEDIATE, 0},				// $89
        {&TXA, ADDR_IMPLICIT, 0},				// $8A
        {&XAA, ADDR_IMMEDIATE, 0},				// $8B
        {&STY, ADDR_ABSOLUTE, 0},				// $8C
        {&STA, ADDR_ABSOLUTE, 0},				// $8D
        {&STX, ADDR_ABSOLUTE, 0},				// $8E
        {&SAX, ADDR_ABSOLUTE, 0},				// $8F
        {&BCC, ADDR_RELATIVE, 0},				// $90
        {&STA, ADDR_INDEXED_INDIRECT_Y, 6},		// $91
        {&STP, ADDR_IMPLICIT, 0},				// $92
        {&AHX, ADDR_INDEXED_INDIRECT_Y, 0},		// $93
        {&STY, ADDR_ZERO_PAGE_INDEXED_X, 0},	// $94
        {&STA, ADDR_ZERO_PAGE_INDEXED_X, 0},	// $95
        {&STX, ADDR_ZERO_PAGE_INDEXED_Y, 0},	// $96
        {&SAX, ADDR_ZERO_PAGE_INDEXED_Y, 0},	// $97
        {&TYA, ADDR_IMPLICIT, 0},				// $98
        {&STA, ADDR_ABSOLUTE_INDEXED_Y, 5},		// $99
        {&TXS, ADDR_IMPLICIT, 0},				// $9A
        {&TAS, ADDR_ABSOLUTE_INDEXED_Y, 0},		// $9B
        {&SHY, ADDR_ABSOLUTE_INDEXED_X, 0},		// $9C
        {&STA, ADDR_ABSOLUTE_INDEXED_X, 5},		// $9D
        {&SHX, ADDR_ABSOLUTE_INDEXED_Y, 0},		// $9E
        {&AHX, ADDR_ABSOLUTE_INDEXED_Y, 0},		// $9F
        {&LDY, ADDR_IMMEDIATE, 0},				// $A0
        {&LDA, ADDR_INDEXED_INDIRECT_X, 0},		// $A1
        {&LDX, ADDR_IMMEDIATE, 0},				// $A2
        {&LAX, ADDR_INDEXED_INDIRECT_X, 0},		// $A3
        {&LDY, ADDR_ZERO_PAGE, 0},				// $A4
        {&LDA, ADDR_ZERO_PAGE, 0},				// $A5
        {&LDX, ADDR_ZERO_PAGE, 0},				// $A6
        {&LAX, ADDR_ZERO_PAGE, 0},				// $A7
        {&TAY, ADDR_IMPLICIT, 0},				// $A8
        {&LDA, ADDR_IMMEDIATE, 0},				// $A9
        {&TAX, ADDR_IMPLICIT, 0},				// $AA
        {&LAX, ADDR_IMMEDIATE, 0},				// $AB
        {&LDY, ADDR_ABSOLUTE, 0},				// $AC
        {&LDA, ADDR_ABSOLUTE, 0},				// $AD
        {&LDX, ADDR_ABSOLUTE, 0},				// $AE
        {&LAX, ADDR_ABSOLUTE, 0},				// $AF
        {&BCS, ADDR_RELATIVE, 0},				// $B0
        {&LDA, ADDR_INDEXED_INDIRECT_Y, 0},		// $B1
        {&STP, ADDR_IMPLICIT, 0},				// $B2
        {&LAX, ADDR_INDEXED_INDIRECT_Y, 0},		// $B3
        {&LDY, ADDR_ZERO_PAGE_INDEXED_X, 0},	// $B4
        {&LDA, ADDR_ZERO_PAGE_INDEXED_X, 0},	// $B5
        {&LDX, ADDR_ZERO_PAGE_INDEXED_Y, 0},	// $B6
        {&LAX, ADDR_ZERO_PAGE_INDEXED_Y, 0},	// $B7
        {&CLV, ADDR_IMPLICIT, 0},				// $B8
        {&LDA, ADDR_ABSOLUTE_INDEXED_Y, 0},		// $B9
        {&TSX, ADDR_IMPLICIT, 0},				// $BA
        {&LAS, ADDR_ABSOLUTE_INDEXED_Y, 0},		// $BB
        {&LDY, ADDR_ABSOLUTE_INDEXED_X, 0},		// $BC
        {&LDA, ADDR_ABSOLUTE_INDEXED_X, 0},		// $BD
        {&LDX, ADDR_ABSOLUTE_INDEXED_Y, 0},		// $BE
        {&LAX, ADDR_ABSOLUTE_INDEXED_Y, 0},		// $BF
        {&CPY, ADDR_IMMEDIATE, 0},				// $C0
        {&CMP, ADDR_INDEXED_INDIRECT_X, 0},		// $C1
        {&NOP, ADDR_IMMEDIATE, 0},				// $C2
        {&DCP, ADDR_INDEXED_INDIRECT_X, 8},		// $C3
        {&CPY, ADDR_ZERO_PAGE, 0},				// $C4
        {&CMP, ADDR_ZERO_PAGE, 0},				// $C5
        {&DEC, ADDR_ZERO_PAGE, 5},				// $C6
        {&DCP, ADDR_ZERO_PAGE, 5},				// $C7
        {&INY, ADDR_IMPLICIT, 0},				// $C8
        {&CMP, ADDR_IMMEDIATE, 0},				// $C9
        {&DEX, ADDR_IMPLICIT, 0},				// $CA
        {&AXS, ADDR_IMMEDIATE, 0},				// $CB
        {&CPY, ADDR_ABSOLUTE, 0},				// $CC
        {&CMP, ADDR_ABSOLUTE, 0},				// $CD
        {&DEC, ADDR_ABSOLUTE, 6},				// $CE
        {&DCP, ADDR_ABSOLUTE, 6},				// $CF
        {&BNE, ADDR_RELATIVE, 0},				// $D0
        {&CMP, ADDR_INDEXED_INDIRECT_Y, 0},		// $D1
        {&STP, ADDR_IMPLICIT, 0},				// $D2
        {&DCP, ADDR_INDEXED_INDIRECT_Y, 8},		// $D3
        {&NOP, ADDR_ZERO_PAGE_INDEXED_X, 0},	// $D4
        {&CMP, ADDR_ZERO_PAGE_INDEXED_X, 0},	// $D5
        {&DEC, ADDR_ZERO_PAGE_INDEXED_X, 6},	// $D6
        {&DCP, ADDR_ZERO_PAGE_INDEXED_X, 6},	// $D7
        {&CLD, ADDR_IMPLICIT, 0},				// $D8
        {&CMP, ADDR_ABSOLUTE_INDEXED_Y, 0},		// $D9
        {&NOP, ADDR_IMPLICIT, 0},				// $DA
        {&DCP, ADDR_ABSOLUTE_INDEXED_Y, 7},		// $DB
        {&NOP, ADDR_ABSOLUTE_INDEXED_X, 0},		// $DC
        {&CMP, ADDR_ABSOLUTE_INDEXED_X, 0},		// $DD
        {&DEC, ADDR_ABSOLUTE_INDEXED_X, 7},		// $DE
        {&DCP, ADDR_ABSOLUTE_INDEXED_X, 7},		// $DF
        {&CPX, ADDR_IMMEDIATE, 0},				// $E0
        {&SBC, ADDR_INDEXED_INDIRECT_X, 0},		// $E1
        {&NOP, ADDR_IMMEDIATE, 0},				// $E2
        {&ISC, ADDR_INDEXED_INDIRECT_X, 8},		// $E3
        {&CPX, ADDR_ZERO_PAGE, 0},				// $E4
        {&SBC, ADDR_ZERO_PAGE, 0},				// $E5
        {&INC, ADDR_ZERO_PAGE, 5},				// $E6
        {&ISC, ADDR_ZERO_PAGE, 5},				// $E7
        {&INX, ADDR_IMPLICIT, 0},				// $E8
        {&SBC, ADDR_IMMEDIATE, 0},				// $E9
        {&NOP, ADDR_IMPLICIT, 0},				// $EA
        {&SBC, ADDR_IMMEDIATE, 0},				// $EB
        {&CPX, ADDR_ABSOLUTE, 0},				// $EC
        {&SBC, ADDR_ABSOLUTE, 0},				// $ED
        {&INC, ADDR_ABSOLUTE, 6},				// $EE
        {&ISC, ADDR_ABSOLUTE, 6},				// $EF
        {&BEQ, ADDR_RELATIVE, 0},				// $F0
        {&SBC, ADDR_INDEXED_INDIRECT_Y, 0},		// $F1
        {&STP, ADDR_IMPLICIT, 0},				// $F2
        {&ISC, ADDR_INDEXED_INDIRECT_Y, 8},		// $F3
        {&NOP, ADDR_ZERO_PAGE_INDEXED_X, 0},	// $F4
        {&SBC, ADDR_ZERO_PAGE_INDEXED_X, 0},	// $F5
        {&INC, ADDR_ZERO_PAGE_INDEXED_X, 6},	// $F6
        {&ISC, ADDR_ZERO_PAGE_INDEXED_X, 6},	// $F7
        {&SED, ADDR_IMPLICIT, 0},				// $F8
        {&SBC, ADDR_ABSOLUTE_INDEXED_Y, 0},		// $F9
        {&NOP, ADDR_IMPLICIT, 0},				// $FA
        {&ISC, ADDR_ABSOLUTE_INDEXED_Y, 7},		// $FB
        {&NOP, ADDR_ABSOLUTE_INDEXED_X, 0},		// $FC
        {&SBC, ADDR_ABSOLUTE_INDEXED_X, 0},		// $FD
        {&INC, ADDR_ABSOLUTE_INDEXED_X, 7},		// $FE
        {&ISC, ADDR_ABSOLUTE_INDEXED_X, 7}		// $FF
    };
};

