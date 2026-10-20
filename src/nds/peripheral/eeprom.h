#pragma once
#include "../bus_types.h"
#include <vector>

namespace nds{

    enum EEPROM_STATE{
        EEPROM_STATE_IDLE,
        EEPROM_STATE_WRITE_ADDRESS,
        EEPROM_STATE_READ_ADDRESS,
        EEPROM_STATE_READ_STATUS,
        EEPROM_STATE_WRITE_STATUS,
        EEPROM_STATE_READ_JEDEC,
        EEPROM_STATE_WRITE,
        EEPROM_STATE_READ,
    };


    class EEPROM{

        const size_t size;
        std::vector<Byte> data;
        const int addressLength;
        const int pageSize;

        Word runningAddress = 0;
        int remainingInput = 0;
        int remainingPage = 0;

        bool isWriteProtected(Word addr);

        
        public:
        Byte status = 0;
        EEPROM_STATE state = EEPROM_STATE_IDLE;
        EEPROM(size_t size);

        void loadSave(const std::vector<Byte> &data);

        std::vector<Byte> getSave();

        void write(Byte data);
        Byte read();
    };
}
