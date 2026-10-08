#pragma once
#include "../bus_types.h"

namespace nds{

    inline uint64_t sign_extend_n_64(uint64_t x, uint64_t bits) {
        uint64_t m = 1u << (bits - 1);
        return (x ^ m) - m;
    }

    class DSMaths{
        HalfWord DIVCNT = 0;
        uint64_t divNumer = 0;
        uint64_t divDenom = 0;
        uint64_t divResult = 0;
        uint64_t divRemResult = 0;

        HalfWord SQRTCNT = 0;
        Word sqrtResult = 0;
        uint64_t sqrtParam = 0;

        int remainingDiv = -1;
        int remainingSqrt = -1;

        void startDivision();
        void startSqrt();

        void doDivision();
        void doSqrt();

        public:

        void clock();

        Byte onRead(Word addr);
        void onWrite(Word addr, Byte val);
    };
}
