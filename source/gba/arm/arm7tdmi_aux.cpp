#include "arm7tdmi.h"
#include <bit>

using namespace gba;

CpuRegisterState gba::CPU::getRegisterState() const
{
    CpuRegisterState state;
    state.R[0] = _R0;
    state.R[1] = _R1;
    state.R[2] = _R2;
    state.R[3] = _R3;
    state.R[4] = _R4;
    state.R[5] = _R5;
    state.R[6] = _R6;
    state.R[7] = _R7;
    state.R[8] = _R8;
    state.R[9] = _R9;
    state.R[10] = _R10;
    state.R[11] = _R11;
    state.R[12] = _R12;
    state.R[13] = _R13_SP;
    state.R[14] = _R14_L_R;
    state.R[15] = _R15_PC;

    state.R_fiq[0] = _R8_fiq;
    state.R_fiq[1] = _R9_fiq;
    state.R_fiq[2] = _R10_fiq;
    state.R_fiq[3] = _R11_fiq;
    state.R_fiq[4] = _R12_fiq;
    state.R_fiq[5] = _R13_fiq;
    state.R_fiq[6] = _R14_fiq;

    state.R_svc[0] = _R13_SVC;
    state.R_svc[1] = _R14_SVC;

    state.R_abt[0] = _R13_abt;
    state.R_abt[1] = _R14_abt;

    state.R_irq[0] = _R13_irq;
    state.R_irq[1] = _R14_irq;

    state.R_und[0] = _R13_und;
    state.R_und[1] = _R14_und;
    
    state.CPSR = _CPSR.raw;

    state.SPSR[0] = _SPSR_fiq.raw;
    state.SPSR[1] = _SPSR_SVC.raw;
    state.SPSR[2] = _SPSR_abt.raw;
    state.SPSR[3] = _SPSR_irq.raw;
    state.SPSR[4] = _SPSR_und.raw;

    state.Pipeline[0] = pipelineDecoded.value_or((InstructionInfo{UnimplementedInstruction, 0})).code;
    state.Pipeline[1] = pipelineRead.value_or(0);

    return state;
}

void gba::CPU::setRegisterState(CpuRegisterState state)
{
    _R0 = state.R[0];
    _R1 = state.R[1];
    _R2 = state.R[2];
    _R3 = state.R[3];
    _R4 = state.R[4];
    _R5 = state.R[5];
    _R6 = state.R[6];
    _R7 = state.R[7];
    _R8 = state.R[8];
    _R9 = state.R[9];
    _R10 = state.R[10];
    _R11 = state.R[11];
    _R12 = state.R[12];
    _R13_SP = state.R[13];
    _R14_L_R = state.R[14];
    _R15_PC = state.R[15];

    _R8_fiq = state.R_fiq[0];
    _R9_fiq = state.R_fiq[1];
    _R10_fiq = state.R_fiq[2];
    _R11_fiq = state.R_fiq[3];
    _R12_fiq = state.R_fiq[4];
    _R13_fiq = state.R_fiq[5];
    _R14_fiq = state.R_fiq[6];

    _R13_SVC = state.R_svc[0];
    _R14_SVC = state.R_svc[1];

    _R13_abt = state.R_abt[0];
    _R14_abt = state.R_abt[1];

    _R13_irq = state.R_irq[0];
    _R14_irq = state.R_irq[1];

    _R13_und = state.R_und[0];
    _R14_und = state.R_und[1];
    
    _CPSR.raw = state.CPSR;

    _SPSR_fiq.raw = state.SPSR[0];
    _SPSR_SVC.raw = state.SPSR[1];
    _SPSR_abt.raw = state.SPSR[2];
    _SPSR_irq.raw = state.SPSR[3];
    _SPSR_und.raw = state.SPSR[4];

    pipelineDecoded = {decodeInstruction(state.Pipeline[0])};
    pipelineRead = {state.Pipeline[1]};
}

InstructionInfo gba::CPU::getInstructionInPipeline()
{
    return this->pipelineDecoded.value();
}

std::string gba::CPU::printCPSR()
{
    std::string res = "";
    auto status = std::bit_cast<StatusRegister>(*this->registerMap[mode()][CPSR]);
    res += "Z: " + std::to_string(status.Z) + "\tC:" + std::to_string(status.C) + "\tN:" + std::to_string(status.N) + "\tV:" + std::to_string(status.V);
    return res;
}

std::string gba::CPU::printMode()
{
    switch(mode()){
        case SystemUser:
            return "SystemUser";
        case FIQ:
            return "FIQ";
        case Supervisor:
            return "Supervisor";
        case AbortMode:
            return "Abort";
        case IRQ:
            return "IRQ";
        case Undefined:
            return "Undefined";
    }
    return "?";
}
