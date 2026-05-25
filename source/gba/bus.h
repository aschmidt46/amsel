#pragma once

#include "arm/arm7tdmi.h"

namespace gba{
    class Bus{
        Byte* accessMemory(Word addr);
        public:
        // Adressen vorher noch alignen?
        void writeByte(Word addr, Byte val);
        Byte readByte(Word addr);
        void writeHalfWord(Word addr, HalfWord val);
        HalfWord readHalfWord(Word addr);
        void writeWord(Word addr, Word val);
        Word readWord(Word addr);
    };
}
