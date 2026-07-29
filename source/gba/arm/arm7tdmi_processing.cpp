#include "arm7tdmi.h"
#include <optional>
#include <iostream>
#include "../test/logging.h"
#include "bus_types.h"
extern "C" {
#include <armdisasm.h>
}

using namespace gba;

Byte gba::CPU::readByte(Word addr)
{
    Byte res = this->bus.lock()->readByte(addr);
    lastTransactionWasRead = true;
    lastTransactionData = res;
    lastTransactionAddress = addr;
    return res;
}

HalfWord gba::CPU::readHalfWord(Word addr)
{
    addr &= ~1u;
    HalfWord res = this->bus.lock()->readHalfWord(addr);
    lastTransactionWasRead = true;
    lastTransactionData = res;
    lastTransactionAddress = addr;
    return res;
}

HalfWord gba::CPU::readHalfWordUnaligned(Word addr)
{
    HalfWord res = this->bus.lock()->readHalfWord(addr);
    lastTransactionWasRead = true;
    lastTransactionData = res;
    lastTransactionAddress = addr;
    return res;
}

Word gba::CPU::readWord(Word addr)
{
    addr &= ~(0b11);
    Word res = this->bus.lock()->readWord(addr);
    lastTransactionWasRead = true;
    lastTransactionData = res;
    lastTransactionAddress = addr;
    return res;
}

Word gba::CPU::readWordUnaligned(Word addr)
{
    Word res = this->bus.lock()->readWord(addr);
    lastTransactionWasRead = true;
    lastTransactionData = res;
    lastTransactionAddress = addr;
    return res;
}

void gba::CPU::writeByte(Word addr, Byte val)
{
    this->bus.lock()->writeByte(addr, val);
    lastTransactionWasRead = false;
    lastTransactionData = val;
    lastTransactionAddress = addr;
}

void gba::CPU::writeHalfWord(Word addr, HalfWord val)
{
    addr &= ~1u;
    this->bus.lock()->writeHalfWord(addr, val);
    lastTransactionWasRead = false;
    lastTransactionData = val;
    lastTransactionAddress = addr;
}

void gba::CPU::writeHalfWordUnaligned(Word addr, HalfWord val)
{
    this->bus.lock()->writeHalfWord(addr, val);
    lastTransactionWasRead = false;
    lastTransactionData = val;
    lastTransactionAddress = addr;
}

void gba::CPU::writeWord(Word addr, Word val)
{
    addr &= ~(0b11);
    this->bus.lock()->writeWord(addr, val);
    lastTransactionWasRead = false;
    lastTransactionData = val;
    lastTransactionAddress = addr;
}

void gba::CPU::writeWordUnaligned(Word addr, Word val)
{
    this->bus.lock()->writeWord(addr, val);
    lastTransactionWasRead = false;
    lastTransactionData = val;
    lastTransactionAddress = addr;
}


void gba::CPU::advanceCPU()
{
    // Die Pipeline MUSS gefüllt sein, bevor IRQ beginnt
    if(!pipelineDecoded.has_value() || !pipelineRead.has_value()){
        advancePipeline();
        return;
    }
    if(pollInterrupts()){
        executeHardwareInterrupt();
        flushPipeline();
    }
    else{
        wasInterrupt = false;
        Word prevPC = _R15_PC;
        bool jump = executeInstruction();
        if(jump && !wasInterrupt && prevPC > 0x01000000 && _R15_PC < 0x01000000){
            std::cout << "Verdächtiger Sprung!\n";
            // bus.lock()->setHalt();
        }
        if(jump){
            flushPipeline();
        }
        else advancePipeline();
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
        else advancePipeline();
    } while(!pipelineIsSaturated());
}

void gba::CPU::clock() {
    if(this->remainingCycles <= 0){
        //Debugging
        if(pipelineDecoded.has_value()){
            circular[circularIndex] = {_R15_PC - (state() == ARM ? 8 : 4), state()};
            incrementCircular();
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
    if(!_CPSR.state.I && (bus.lock()->readByte(0x4000208) & 1u)){
        HalfWord IE = this->bus.lock()->readHalfWord(0x4000200);
        HalfWord IF = this->bus.lock()->readHalfWord(0x4000202);
        for(int i = 0; i < 14; i++){
            if((IE & (1u << i)) && (IF & (1u << i))){
                return true;
            }
        }
    }
    return false;
}

bool gba::CPU::pipelineIsSaturated()
{
    return pipelineDecoded.has_value() && pipelineRead.has_value();
}

void gba::CPU::flushPipeline()
{
    this->pipelineRead = {};
    this->pipelineDecoded = {};
}

void gba::CPU::advancePipeline()
{
    Word pcOffset = this->state() == ARM ? 4 : 2;
    pipelineDecoded = pipelineRead.and_then([this](auto val) -> std::optional<InstructionInfo> {return {this->decodeInstruction(val)};});
    pipelineRead = this->state() == ARM ? this->readWord(*registerMap[mode()][R15]) : this->readHalfWord(*registerMap[mode()][R15]);
    *registerMap[mode()][R15] += pcOffset;
}

bool gba::CPU::advancedThisClock() {
    return advanced;
}

bool gba::CPU::pipelineHasValue(){
    return pipelineDecoded.has_value();
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
        Word opcode = this->bus.lock()->readWord(pc & ~mask);
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
        Word opcode = this->bus.lock()->readWord(pc.first & ~mask);
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
        disasm_arm(&s, pipelineDecoded.value_or(InstructionInfo{ InstructionType::UnimplementedInstruction, 0}).code);
    }
    else{
        auto code = pipelineDecoded.value_or(InstructionInfo{ InstructionType::UnimplementedInstruction, 0}).code;
        disasm_thumb(&s, HalfWord(code), HalfWord(code >> 16));
    }
    std::string text = s.text;
    return text.substr(0, text.find(' '));
}
