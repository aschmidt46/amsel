#pragma once
#include "../arm/arm7tdmi.h"


void separateEveryN(std::string& str, int interval, const char* sep);

std::string thex(uintptr_t input);
std::string thexNorm(std::string s, int n);
std::string getHex(size_t input, int length);

std::string printDiff(gba::CpuRegisterState was, gba::CpuRegisterState is, gba::CpuRegisterState test);
