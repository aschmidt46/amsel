#pragma once

#include "arm/bus_types.h"
#include "gba/arm/bus_types.h"
#include <vector>

namespace gba{
    struct InstructionInfo;

    class IBus{
        public:
        std::vector<InstructionInfo> armCache;
        std::vector<InstructionInfo> thumbCache;
        std::vector<InstructionInfo> armCacheBIOS;
        std::vector<InstructionInfo> thumbCacheBIOS;
        virtual void writeByte(Word addr, Byte val) = 0;
        virtual Byte readByte(Word addr) = 0;
        virtual void writeHalfWord(Word addr, HalfWord val) = 0;
        virtual HalfWord readHalfWord(Word addr) = 0;
        virtual void writeWord(Word addr, Word val) = 0;
        virtual Word readWord(Word addr) = 0;

        virtual HalfWord getIE() = 0;
        virtual HalfWord getIF() = 0;
        virtual bool hasIME() = 0;

        virtual void setHalt(){};
    };
}