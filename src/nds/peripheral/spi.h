#pragma once
#include "../bus_types.h"
#include "firmware.h"

namespace nds{

    union SPI_CNT_T{

        struct{
            HalfWord baudRate : 2;
            HalfWord _unused : 5;
            HalfWord busyFlag : 1;
            HalfWord deviceSelect : 2;
            HalfWord transferSize : 1;
            HalfWord chipSelectHold : 1;
            HalfWord _unused2 : 2;
            HalfWord interruptRequest : 1;
            HalfWord spiBusEnable : 1;
        } state;
        HalfWord raw;

        inline Word getBaudrate() const{
            return 0;
        };
    };

    class Bus7;

    class SPI{
        Bus7* bus;
        Firmware firmware;

        SPI_CNT_T SPICNT = {.raw = 0};
        Byte SPIDATA = 0;

        void startTransfer();
        public:
        SPI() = default;
        SPI(Bus7* bus) : bus(bus){};

        void onWrite(Word addr, Byte val);
        Byte onRead(Word addr);
        void completeTransfer();
        void setFirmware(const std::vector<Byte> &data);

    };

}

