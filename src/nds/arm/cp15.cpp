#include "cp15.h"

using namespace nds;

void nds::CP15::MCR(Word opcode, Word armRegister, Word coprocRegister, Word operandRegister)
{
}

Word nds::CP15::MRC(Word opcode, Word coprocRegister, Word operandRegister)
{
    return Word();
}
