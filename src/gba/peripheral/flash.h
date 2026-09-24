#pragma once
#include "../arm/bus_types.h"
#include <array>
#include <vector>

namespace gba{

    enum FlashState{
        IDLE,
        PREPARE_1,
        PREPARE_2,
        RECEIVE_ERASE,
        ERASE_PREPARE_1,
        ERASE_PREPARE_2,
        AWAIT_WRITE,
        AWAIT_BANK_SWITCH,
    };
    class Flash{
        std::vector<Byte> memory;
        size_t SIZE = 0;
        FlashState state = IDLE;
        bool chipIdent = false;

        Word bank = 0;

        void parseCommand(Word addr, Byte val);
        void OnWrite5555(Byte val);
        void OnWrite2AAA(Byte val);
        void OnWriteN000(Word addr, Byte val);

        public:
        Flash(size_t size){
            memory = std::vector<Byte>(size, 0xFF);
            SIZE = size;
        }
        Byte OnRead(Word addr);
        void OnWrite(Word addr, Byte val);
        void loadSave(const std::vector<uint8_t> &saveData);
        std::vector<Byte> getSaveData();
    };
}

