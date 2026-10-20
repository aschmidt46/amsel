#pragma once
#include <array>
#include <vector>
#include "../bus_types.h"
#include <fstream>

namespace nds{

    enum FirmwareState{
        FIRMWARE_IDLE,
        FIRMWARE_READ_STATUS,
        FIRMWARE_READ_ID,
        FIRMWARE_READ_DATA_ADDRESS,
        FIRMWARE_READ_DATA,
        FIRMWARE_PW_ADDRESS,
        FIRMWARE_PW
    };

    class Bus7;

    class Firmware{

        std::array<Byte, 3> ID = {0x20, 0x40, 0x12};

        
        Word address = 0;
        int remainingBytes = 0;
        
        
        size_t readWriteIndex = 0;
        Word origPage = 0;
        Bus7* bus;

        
        public:
        bool awaitRead = false; // Hack damit nicht extra Byte zu SPI gesendet wird, bevor tatsächlich gelesen wird
        // std::ofstream cout;
        Firmware(Bus7* bus) : bus(bus){
            // cout = std::ofstream("firmware.log");
        };

        Byte status = 0;
        std::vector<Byte> firmwareData = {};
        FirmwareState state = FIRMWARE_IDLE;

        void write(Byte data);
        Byte read();
    };
}
