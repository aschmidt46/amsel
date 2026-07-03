#pragma once
#include "arm/bus_types.h"

namespace gba{
    class DMAChannel{
        int dmaIndex;
    
        public:
        DMAChannel(int index);
        void onWrite(Word addr, Byte val);
        Byte onRead(Word addr);
    };
}
