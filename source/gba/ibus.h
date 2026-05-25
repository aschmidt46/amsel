#pragma once

#include "arm/arm7tdmi.h"

namespace gba{
    class IBus{
        public:
        virtual void writeByte(Word addr, Byte val) = 0;
        virtual Byte readByte(Word addr) = 0;
        virtual void writeHalfWord(Word addr, HalfWord val) = 0;
        virtual HalfWord readHalfWord(Word addr) = 0;
        virtual void writeWord(Word addr, Word val) = 0;
        virtual Word readWord(Word addr) = 0;
    };
}