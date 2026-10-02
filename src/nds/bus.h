#pragma once
#include "bus7.h"
#include "bus9.h"
#include "../gba/register/general_purpose.h"

namespace nds{

    class SharedBus{
        bool isHalted = false;
        Word IE = 0;
        Word IF = 0;
        gba::GeneralPurpose32 IME;
        public:
        std::shared_ptr<Bus7> bus7;
        std::shared_ptr<Bus9> bus9;
        void init();
        void setIF(int bit, bool val);

        Word getIE();
        Word getIF();
        bool hasIME();

        void setHalt();
    };

}
