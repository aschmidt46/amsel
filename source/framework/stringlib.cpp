#include "stringlib.h"
#include <format>
#include <algorithm>



std::string hex(size_t input)
{
    std::string str = std::format("{:x}", input);
    std::transform(str.begin(), str.end(), str.begin(), ::toupper);
    return str;
}

std::string hexNorm(std::string s, size_t n)
{
    while ((int)s.size() < n)
        s = "0" + s;
    return s;
}

std::string getHex0x(size_t input, size_t length){
    return "0x"+hexNorm(hex(input), length);
}

std::string getHexDollar(size_t input, size_t length){
    return "$"+hexNorm(hex(input), length);
}
