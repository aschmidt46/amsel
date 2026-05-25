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
    return this->bus.lock()->readHalfWord(addr);
}

Word gba::CPU::readWord(Word addr)
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
    this->bus.lock()->writeWord(addr, val);
}


void gba::CPU::advanceCPU()
{
    if(shouldFlush){
        flushPipeline();
        shouldFlush = false;
    }

    // 3 Stage Pipeline
    // Beginne mit aktuellem Zustand
    bool jump = executeInstruction();
    if(jump){
        shouldFlush = true;
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
    return pipelineDecoded.has_value() && pipelineRead.has_value() && !shouldFlush;
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
    pipelineRead = this->readWord(*registerMap[mode()][R15]);
    *registerMap[mode()][R15] += pcOffset;
}
