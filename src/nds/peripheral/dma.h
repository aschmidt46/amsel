#pragma once
#include "../bus_types.h"
#include "gba/register/general_purpose.h"
#include <memory>
#include "../scheduler.h"


namespace nds{
    class SharedBus;

    enum DestAddrControl : HalfWord{
        DEST_INCREMENT = 0,
        DEST_DESCREMENT = 1,
        DEST_FIXED = 2,
        DEST_INCREMENT_RELOAD = 3,
    };

    enum SrcAddrControl : HalfWord{
        SRC_INCREMENT = 0,
        SRC_DESCREMENT = 1,
        SRC_FIXED = 2,
        SRC_PROHIBITED = 3,
    };

    enum DMAStartTiming : HalfWord{
        DMA_IMMEDIATE = 0,
        DMA_VBLANK = 1,
        DMA_HBLANK = 2,
        DMA_SPECIAL_PROHIBITED = 3,
        DMA_SOUND_FIFO = 4,
        DMA_VIDEO_CAPTURE = 5,
    };

    template<typename ConcreteBus>
    class DMAChannel{
        SharedBus* bus;
        ConcreteBus* cbus;
        int dmaIndex;
        const EventType schedulerEvent = NDSEVENT_DmaTransfer7;

        gba::GeneralPurpose32 SourceAddress;
        public: gba::GeneralPurpose32 DestinationAddress;
        private: gba::GeneralPurpose16 WordCount;
        public: gba::GeneralPurpose16 Control;
        
        Word maxCount = 0;
        private:
        Word currentSourceAddr = 0;
        Word currentDestAddr = 0;
        Word currentCount = 0;

        Word lastRead = 0;

        // int remainingCycles = 0;

        // Vorberechnete Derivate
        DMAStartTiming startTiming = DMA_IMMEDIATE;
        int destIncrement = 0;
        int sourceIncrement = 0;

        void resetInternalCounters(bool SAD, bool DAD);
        void printStartTiming();
    
        public:
        // bool isActive = false;
        DMAChannel() : SourceAddress(0), DestinationAddress(0), WordCount(0), Control(0){};
        DMAChannel(int index, SharedBus* busPtr, ConcreteBus* cbus, EventType event);
        void onWrite(Word addr, Byte val);
        Byte onRead(Word addr);


        void commenceTransfer();

        inline DestAddrControl getDestAddrControl() const{
            return DestAddrControl((Control.raw >> 5) & 0b11u);
        }

        inline SrcAddrControl getSrcAddrControl() const{
            return SrcAddrControl((Control.raw >> 7) & 0b11u);
        }

        inline bool doesRepeat() const{
            return Control.raw & (1u << 9);
        }

        inline bool dmaTransferIs32Bit() const{
            return Control.raw & (1u << 10);
        }

        inline bool gamepakDRQ() const{
            return Control.raw & (1u << 11);
        }

        inline DMAStartTiming getStartTiming() const{
            DMAStartTiming timing = DMAStartTiming((Control.raw >> 12) & 0b11u);
            if((dmaIndex == 1 || dmaIndex == 2) && timing == DMA_SPECIAL_PROHIBITED){
                timing = DMA_SOUND_FIFO;
            }
            else if(dmaIndex == 3 && timing == DMA_SPECIAL_PROHIBITED){
                timing = DMA_VIDEO_CAPTURE;
            }
            return timing;
        }

        inline bool irqOnEnd() const{
            return Control.raw & (1u << 14);
        }

        inline bool isEnabled() const{
            return Control.raw & (1u << 15);
        }
    };
}
