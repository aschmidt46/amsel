#include "arm7tdmi.h"
#include <optional>
#include <iostream>

using namespace gba;

Byte gba::CPU::readByte(Word addr)
{
    return this->bus.lock()->readByte(addr);
}

HalfWord gba::CPU::readHalfWord(Word addr)
{
    addr &= ~1u;
    return this->bus.lock()->readHalfWord(addr);
}

HalfWord gba::CPU::readHalfWordUnaligned(Word addr)
{
    return this->bus.lock()->readHalfWord(addr);
}

Word gba::CPU::readWord(Word addr)
{
    addr &= ~(0b11);
    return this->bus.lock()->readWord(addr);
}

Word gba::CPU::readWordUnaligned(Word addr)
{
    return this->bus.lock()->readWord(addr);
}

void gba::CPU::writeByte(Word addr, Byte val)
{
    this->bus.lock()->writeByte(addr, val);
}

void gba::CPU::writeHalfWord(Word addr, HalfWord val)
{
    this->bus.lock()->writeHalfWord(addr, val);
}

void gba::CPU::writeWord(Word addr, Word val)
{
    addr &= ~(0b11);
    this->bus.lock()->writeWord(addr, val);
}

void gba::CPU::writeWordUnaligned(Word addr, Word val)
{
    this->bus.lock()->writeWord(addr, val);
}


void gba::CPU::advanceCPU()
{

    // 3 Stage Pipeline
    // Beginne mit aktuellem Zustand
    bool jump = executeInstruction();
    if(jump){
        flushPipeline();
    }
    else advancePipeline();
}

void gba::CPU::advanceCPUToNextValidState()
{
    do {
        advanceCPU();
    } while(!pipelineIsSaturated());
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
