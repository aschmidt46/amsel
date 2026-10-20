#pragma once
#include "../bus_types.h"
#include <vector>
#include "../scheduler.h"
#include "../resources/rominfo.h"
#include <memory>
#include "eeprom.h"

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

    union AUXSPICNT_T{
        struct {
            HalfWord baudRate : 2;
            HalfWord _unused0 : 4;
            HalfWord holdChipSelect : 1;
            HalfWord busy : 1;
            HalfWord _unused1 : 5;
            HalfWord ndsSlotMode : 1;
            HalfWord transferReadyIRQ : 1;
            HalfWord ndsSlotEnable : 1;
        } state;
        HalfWord raw = 0;
    };

    enum GameCardEventType : int{
        GameCardEventType_DataWord = 0,
        GameCardEventType_ChipID = 1,
        GameCardEventType_Dummy = 2,
        GameCardEventType_SecureBlock = 3,
        GameCardEventType_Dummy_Chip = 4,
    };

    class SharedBus;

    class Cartridge{
        SharedBus* bus;
        public:
        std::vector<Byte> cartridge;
        Byte* secureArea = nullptr;
        Word cartID = 0;
        std::unique_ptr<EEPROM> eeprom = nullptr;
        ROMCTRL_T romControl = {.raw = 0};
        int currentMode = 0;
        AUXSPICNT_T AUXSPICNT = {.raw = 0};
        private:
        Byte AUXSPIDATA = 0;
        
        
        uint64_t gameCardBusCommandOut = 0;
        
        Word gameCardBus4byteDataIn = 0;
        
        int64_t remainingLength = 0;
        Word offsetAddress = 0;
        bool oncePerCycle = false;
        bool useKEY1 = false;

        inline Byte getCMD(){
            uint64_t tmp = gameCardBusCommandOut;

            if(useKEY1){
                decrypt64((Byte*)&tmp);

                if((tmp >> 60) == 0x4) return 4;
                if((tmp >> 60) == 0x1) return 1;
                if((tmp >> 60) == 0xA) return 0xA;
            }

            return tmp >> 56;
        }

        void startTransfer();

        RomInfo romInfo = {0, SAVETYPE_NONE, 0};

        void unselectDevices();
        void startTransferSPI();


        public:
        Cartridge() = default;
        Cartridge(SharedBus* bus, const std::vector<Byte> &cart, bool hle = true);
        Byte onRead(Word addr);
        void onWrite(Word addr, Byte val);
        void checkAndResend();
        
        void completeTransfer();
        
        bool canSave();
        std::vector<Byte> getSaveData();
        size_t getSaveSize();
        void loadSave(const std::vector<Byte> &saveData);




        void cardReply(int index);

        void sendDataWord();
        void sendChipID();
        void sendDummy();
        void sendSecureBlock();

        std::array<Byte, 0x1048> keybuf = {};
        std::array<Byte, 0x10> keycode = {};


        void encrypt64(Byte* ptr);
        void decrypt64(Byte* ptr);

        void applyKeycode(Word modulo);

        void initKeycode(Word idcode, Word level, Word modulo);
    };
}
