#pragma once
#include "../arm/arm7tdmi.h"


void separateEveryN(std::string& str, int interval, const char* sep);

std::string printState(gba::CpuRegisterState);

std::string printDiff(gba::CpuRegisterState was, gba::CpuRegisterState is, gba::CpuRegisterState test);
std::string printTransactions(const std::vector<gba::Transaction> &transactions);

std::string disassemble_code(gba::CpuRegisterState state, uint32_t code);
