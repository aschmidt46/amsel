#pragma once

#include "gba/arm/bus_types.h"
namespace gba{
    struct GeneralPurpose32{
        Word start;
        Word raw;
        Byte OnReadByte(Word addr);
        void OnWriteByte(Word addr, Byte val);
        GeneralPurpose32(Word start) : start(start), raw(0) {};
    };
    
    struct GeneralPurpose16{
        Word start;
        HalfWord raw;
        Byte OnReadByte(Word addr);
        void OnWriteByte(Word addr, Byte val);
        GeneralPurpose16(Word start) : start(start), raw(0) {};
    };
}
