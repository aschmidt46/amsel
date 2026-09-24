#pragma once
#include <vector>
#include "../arm/bus_types.h"

namespace gba{
    class Bus;

    enum EEPROMState{
        EEPROM_IDLE,
        EEPROM_READ_OR_WRITE,
        EEPROM_READ_ADDRESS,
        EEPROM_WRITE_ADDRESS,
        EEPROM_READ,
        EEPROM_READ_FILLER,
        EEPROM_WRITE,
    };


    class EEPROM{
        std::vector<uint64_t> memory;
        bool initialized = false;
        EEPROMState state = EEPROM_IDLE;
        uint64_t buffer = 0;
        size_t counter = 0;
        size_t outputctr = 0;
        Bus* bus;
        void setSize(size_t size);

        size_t address = 0;

        public:
        EEPROM(Bus* bus) : bus(bus){}
        void OnWrite(Byte val);
        Byte OnRead();
        void loadSave(const std::vector<uint8_t> &saveData);
        std::vector<Byte> getSaveData();
        size_t getSaveDataSize();
    };

}
