#pragma once
#include "../bus_types.h"

namespace nds{
    enum SaveType{
        SAVETYPE_NONE,
        SAVETYPE_EEPROM,
        SAVETYPE_FLASH,
    };
    struct RomInfo{
        Word id;
        SaveType type;
        Word size;
    };
}
