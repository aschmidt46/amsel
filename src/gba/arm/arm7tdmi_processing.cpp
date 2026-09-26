#include "arm7tdmi.h"
#include <optional>
#include <iostream>
#include "../test/logging.h"
#include "bus_types.h"
#include "gba/arm/arm7tdmi_types.h"
extern "C" {
#include <armdisasm.h>
}

using namespace gba;

Byte gba::CPU::readByte(Word addr)
{
    Byte res = this->bus->readByte(addr);
    lastTransactionWasRead = true;
    lastTransactionData = res;
    lastTransactionAddress = addr;
    return res;
}

HalfWord gba::CPU::readHalfWord(Word addr)
{
    addr &= ~1u;
    HalfWord res = this->bus->readHalfWord(addr);
    lastTransactionWasRead = true;
    lastTransactionData = res;
    lastTransactionAddress = addr;
    return res;
}

HalfWord gba::CPU::readHalfWordUnaligned(Word addr)
{
    HalfWord res = this->bus->readHalfWord(addr);
    lastTransactionWasRead = true;
    lastTransactionData = res;
    lastTransactionAddress = addr;
    return res;
}

Word gba::CPU::readWord(Word addr)
{
    addr &= ~(0b11);
    Word res = this->bus->readWord(addr);
    lastTransactionWasRead = true;
    lastTransactionData = res;
    lastTransactionAddress = addr;
    return res;
}

Word gba::CPU::readWordUnaligned(Word addr)
{
    Word res = this->bus->readWord(addr);
    lastTransactionWasRead = true;
    lastTransactionData = res;
    lastTransactionAddress = addr;
    return res;
}

void gba::CPU::writeByte(Word addr, Byte val)
{
    this->bus->writeByte(addr, val);
    lastTransactionWasRead = false;
    lastTransactionData = val;
    lastTransactionAddress = addr;
}

void gba::CPU::writeHalfWord(Word addr, HalfWord val)
{
    addr &= ~1u;
    this->bus->writeHalfWord(addr, val);
    lastTransactionWasRead = false;
    lastTransactionData = val;
    lastTransactionAddress = addr;
}

void gba::CPU::writeHalfWordUnaligned(Word addr, HalfWord val)
{
    this->bus->writeHalfWord(addr, val);
    lastTransactionWasRead = false;
    lastTransactionData = val;
    lastTransactionAddress = addr;
}

void gba::CPU::writeWord(Word addr, Word val)
{
    addr &= ~(0b11);
    this->bus->writeWord(addr, val);
    lastTransactionWasRead = false;
    lastTransactionData = val;
    lastTransactionAddress = addr;
}

void gba::CPU::writeWordUnaligned(Word addr, Word val)
{
    this->bus->writeWord(addr, val);
    lastTransactionWasRead = false;
    lastTransactionData = val;
    lastTransactionAddress = addr;
}


void gba::CPU::advanceCPU()
{
    // Die Pipeline MUSS gefüllt sein, bevor IRQ beginnt
    if((pipeline[0].type == PipelineEmpty) || (pipeline[1].type == PipelineEmpty)){
        if(state() == ARM)
            advancePipeline<true>();
        else
            advancePipeline<false>();
        return;
    }
    if(pollInterrupts()){
        executeHardwareInterrupt();
        flushPipeline();
    }
    else{
        wasInterrupt = false;
        bool jump = executeInstruction();
        if(jump){
            flushPipeline();
        }
        else{
            if(state() == ARM)
                advancePipeline<true>();
            else
                advancePipeline<false>();
        }
    }
}

// Fürs Testen
void gba::CPU::advanceCPUToNextValidState()
{
    do {
        // 3 Stage Pipeline
        // Beginne mit aktuellem Zustand
        bool jump = executeInstruction();
        if(jump){
            flushPipeline();
        }
        else{
            if(state() == ARM)
                advancePipeline<true>();
            else
                advancePipeline<false>();
        }
    } while(!pipelineIsSaturated());
}

void gba::CPU::clock() {
    if(this->remainingCycles <= 0){
        //Debugging
        if(debug){
            if(pipeline[0].type != PipelineEmpty){
                circular[circularIndex] = {_R15_PC - (state() == ARM ? 8 : 4), state()};
                incrementCircular();
            }
        }
        
        
        this->advanceCPU();
        advanced = true;
    }
    else {
        remainingCycles--;
        advanced = false;
    }
}

bool gba::CPU::pollInterrupts()
{
    //interrupt disable nicht gesetzt             ime gesetzt
    if(!_CPSR.state.I && bus->hasIME()){
        HalfWord IE = this->bus->getIE();
        HalfWord IF = this->bus->getIF();
        return (IE & 0x3FFF) & (IF & 0x3FFF);
    }
    return false;
}

bool gba::CPU::pipelineIsSaturated()
{
    return pipeline[0].type != PipelineEmpty && pipeline[1].type != PipelineEmpty;
}

void gba::CPU::flushPipeline()
{
    this->pipeline[1] = {PipelineEmpty, 0};
    this->pipeline[0] = {PipelineEmpty, 0};
}

void gba::CPU::addCycles(size_t cycles)
{
    remainingCycles += cycles;
}

template<bool isARM>
void gba::CPU::advancePipeline()
{
    if(pipeline[1].type != PipelineEmpty){
        pipeline[0] = pipeline[1];
    }
    else pipeline[0] = {PipelineEmpty, 0};

    // vllt jetzt falsch, weil ich schon im Read cycle dekodiere?
    if(_R15_PC < 0x08000000 && _R15_PC >= 0x4000){
        pipeline[1] = isARM ? decodeInstruction(this->readWord(_R15_PC)) : decodeInstruction(this->readHalfWord(_R15_PC));
        _R15_PC += isARM ? 4 : 2;
        return;
    }
    Word altPC = _R15_PC - 0x08000000;
    if(isARM){
        if(altPC >> 2 >= armCacheSize){
            altPC -= 0x02000000;
        }
        if(altPC >> 2 >= armCacheSize){
            altPC -= 0x02000000;
        }
        std::vector<InstructionInfo>* cache = &this->bus->armCache;
        Word cacheSize = armCacheSize;
        if(_R15_PC < 0x4000){
            cache = &this->bus->armCacheBIOS;
            cacheSize = armCacheSizeBIOS;
            altPC = _R15_PC;
        }
        altPC >>= 2;

        if(altPC < cacheSize &&  (*cache)[altPC].type != PipelineEmpty){
            pipeline[1] = (*cache)[altPC];    
        }
        else{
            pipeline[1] = decodeInstruction(this->readWord(_R15_PC));
            if(altPC < cacheSize){
                // std::cout << "gecacht\n";
                (*cache)[altPC] = pipeline[1];
            }
        }
    }
    else{
        if(altPC >> 1 >= thumbCacheSize){
            altPC -= 0x02000000;
        }
        if(altPC >> 1 >= thumbCacheSize){
            altPC -= 0x02000000;
        }
        std::vector<InstructionInfo>* cache = &this->bus->thumbCache;
        Word cacheSize = thumbCacheSize;
        if(_R15_PC < 0x4000){
            cache = &this->bus->thumbCacheBIOS;
            cacheSize = thumbCacheSizeBIOS;
            altPC = _R15_PC;
        }
        altPC >>= 1;

        if(altPC < cacheSize && (*cache)[altPC].type != PipelineEmpty){
            pipeline[1] = (*cache)[altPC];    
        }
        else{
            pipeline[1] = decodeInstruction(this->readHalfWord(_R15_PC));
            if(altPC < cacheSize){
                // std::cout << "gecacht\n";
                (*cache)[altPC] = pipeline[1];
            }
        }
    }
    _R15_PC += isARM ? 4 : 2;
}

bool gba::CPU::advancedThisClock() {
    return advanced;
}

bool gba::CPU::pipelineHasValue(){
    return pipeline[0].type != PipelineEmpty;
}

std::pair<std::string, std::vector<int>> CPU::getNextNInstructions(int n)
{
    int offset = (state() == ARM ? 4 : 2);
    Word pc = _R15_PC - 2 * offset;
    std::string res = "";
    std::vector<int> length;
    ARMSTATE s;
    s.address = pc;
    disasm_init(&s, DISASM_ADDRESS);

    // Dummy Read, sonst ist Adresse falsch, unbekannt warum
    disasm_arm(&s, 0);
    
    for(int i = 0; i < n; i++){
        Word mask = state() == ARM ? 0b11 : 0b1;
        Word opcode = this->bus->readWord(pc & ~mask);
        s.address = pc - 4;
        if(state() == ARM){
            s.arm_mode = 1;
            disasm_arm(&s, opcode);
        }
        else{
            s.arm_mode = 0;
            disasm_thumb(&s, HalfWord(opcode), HalfWord(opcode >> 16));
        }
        res+="$"+std::string(s.text)+"\n";
        length.push_back(offset);
        pc += offset;
    }
    disasm_cleanup(&s);
    return {res, length};
}

std::pair<std::string, std::vector<int>> CPU::getPrev10Instructions()
{
    int offset = (state() == ARM ? 4 : 2);
    Word mask = state() == ARM ? 0b11 : 0b1;
    std::vector<std::pair<Word, Word>> pcList;
    int it = circularIndex;
    for(int i = 0; i < circSize; i++){

        auto instruction = circular[it];

        if(instruction.first >= 0)
            pcList.push_back(instruction);


        if(it>=circSize-1){
            it = 0;
        }
        else{
            it++;
        }
    }

    std::string res = "";
    std::vector<int> length;
    ARMSTATE s;
    disasm_init(&s, DISASM_ADDRESS);
    if(pcList.size() > 0){
        s.address = pcList[0].first;
        disasm_arm(&s, 0);
    }
    for(const auto &pc : pcList){
        std::string str = "";
        s.address = pc.first - offset;
        Word opcode = this->bus->readWord(pc.first & ~mask);
        if(pc.second == ARM){
            s.arm_mode = 1;
            disasm_arm(&s, opcode);
        }
        else{
            s.arm_mode = 0;
            disasm_thumb(&s, HalfWord(opcode), HalfWord(opcode >> 16));
        }
        res+="$"+std::string(s.text)+"\n";
        length.push_back(pc.second == ARM ? 4 : 2);
    }

    disasm_cleanup(&s);
    return {res, length};
}

void gba::CPU::incrementCircular() {
    circularIndex += 1;
    circularIndex %= circSize;
}

std::string gba::CPU::getCurrentOpcode() {
    ARMSTATE s;
    disasm_init(&s, 0);
    if(state()==ARM){
        disasm_arm(&s, pipeline[0].code);
    }
    else{
        auto code = pipeline[0].code;
        disasm_thumb(&s, HalfWord(code), HalfWord(code >> 16));
    }
    std::string text = s.text;
    return text.substr(0, text.find(' '));
}
