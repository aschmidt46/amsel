#pragma once
#include <string>
#include <cstddef>

std::string hex(size_t input);

std::string hexNorm(std::string s, size_t n);

std::string getHex0x(size_t input, size_t length);

std::string getHexDollar(size_t input, size_t length);
