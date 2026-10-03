#pragma once
#include "../bus_types.h"

namespace nds{
    class CP15{
        public:
        void MCR(Word opcode, Word armRegister, Word coprocRegister, Word operandRegister);
        Word MRC(Word opcode, Word coprocRegister, Word operandRegister);
    };
}
