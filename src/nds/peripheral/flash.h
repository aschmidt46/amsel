#pragma once
#include <array>
#include <vector>
#include "../bus_types.h"
#include <fstream>

namespace nds{

    enum FlashState{
        FLASH_IDLE,
        FLASH_READ_STATUS,
        FLASH_READ_ID,
        FLASH_READ_DATA_ADDRESS,
        FLASH_READ_DATA_ADDRESS_FAST,
        FLASH_READ_DATA,
        FLASH_PW_ADDRESS,
        FLASH_PW,
        FLASH_PP_ADDRESS,
        FLASH_PP,
        FLASH_PE_ADDRESS,
        FLASH_SE_ADDRESS,
        FLASH_DP,
    };

    class Flash{

        std::array<Byte, 3> ID = {0x20, 0x40, 0x12};

        
        Word address = 0;
        int remainingBytes = 0;
        
        
        size_t readWriteIndex = 0;
        Word origPage = 0;

        
        public:
        std::vector<Byte> flashData = {};
        bool awaitRead = false; // Hack damit nicht extra Byte zu SPI gesendet wird, bevor tatsächlich gelesen wird
        Flash(size_t size) : flashData(size, 0xFF){};

        Byte status = 0;
        FlashState state = FLASH_IDLE;

        void loadSave(const std::vector<Byte> &data);
        std::vector<Byte> getSave();

        void write(Byte data);
        Byte read();
    };
}
