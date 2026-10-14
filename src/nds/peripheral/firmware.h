#pragma once
#include <array>
#include <vector>
#include "../bus_types.h"

namespace nds{

    enum FirmwareState{
        FIRMWARE_IDLE,
        FIRMWARE_READ_STATUS,
        FIRMWARE_READ_ID,
        FIRMWARE_READ_DATA_ADDRESS,
        FIRMWARE_READ_DATA,
    };

    class Firmware{

        std::array<Byte, 3> ID = {0x20, 0x40, 0x12};

        Byte status = 0;

        Word address = 0;
        
        
        size_t readWriteIndex = 0;
        
        public:
        std::vector<Byte> firmwareData = {};
        FirmwareState state = FIRMWARE_IDLE;

        void write(Byte data);
        Byte read();
    };
}
