#pragma once
#include "../bus_types.h"
#include <vector>
#include "../scheduler.h"

namespace nds{

    union ROMCTRL_T{
        struct {
            Word key1Gap1Length : 13;
            Word key2EncryptData : 1;
            Word seUnknown : 1;
            Word key2ApplySeed : 1;
            Word key1Gap2Length : 6;
            Word key2EncryptCmd : 1;
            Word dataWordStatus : 1;
            Word dataBlockSize : 3;
            Word transferCLKRate : 1;
            Word key1GapCLKs : 1;
            Word resbReleaseReset : 1;
            Word dataDirectionIsWrite : 1;
            Word blockStartStatus : 1;
        } state;
        Word raw = 0;

        Word getTranferSize() const{
            if(state.dataBlockSize == 0) return 0;
            if(state.dataBlockSize == 7) return 4;
            return 0x100 << state.dataBlockSize;
        }
    };

    class SharedBus;

    class Cartridge{
        SharedBus* bus;
        public:
        std::vector<Byte> cartridge;
        private:
        EventType currentMode = NDSEVENT_None;
        HalfWord AUXSPICNT = 0;
        Byte AUXSPIDATA = 0;

        ROMCTRL_T romControl = {.state = {.key1Gap1Length = 0x08F8, .key1Gap2Length = 0x18}};

        uint64_t gameCardBusCommandOut = 0;

        Word gameCardBus4byteDataIn = 0;

        int64_t remainingLength = 0;
        Word offsetAddress = 0;
        bool oncePerCycle = false;

        inline Byte getCMD() const{
            return gameCardBusCommandOut >> 56;
        }

        void startTransfer();

        public:
        Cartridge() = default;
        Cartridge(SharedBus* bus, const std::vector<Byte> &cart);
        Byte onRead(Word addr);
        void onWrite(Word addr, Byte val);
        void sendDataWord();
        void sendChipID();
        void checkAndResend();
    };
}
