#pragma once
#include "arm/bus_types.h"
#include "gba/arm/bus_types.h"
#include "register/general_purpose.h"
#include <memory>


namespace gba{
    class Bus;

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

    class DMAChannel{
        std::weak_ptr<Bus> bus;
        int dmaIndex;

        GeneralPurpose32 SourceAddress;
        GeneralPurpose32 DestinationAddress;
        GeneralPurpose16 WordCount;
        public: GeneralPurpose16 Control;
        
        private:
        Word currentSourceAddr = 0;
        Word currentDestAddr = 0;
        Word currentCount = 0;
        Word maxCount = 0;

        int remainingCycles = 0;

        void resetInternalCounters(bool SAD, bool DAD);
    
        public:
        bool isActive = false;
        DMAChannel(int index, std::weak_ptr<Bus> busPtr);
        void onWrite(Word addr, Byte val);
        Byte onRead(Word addr);

        // war aktiv / hat etwas getan
        bool clock();

        DestAddrControl getDestAddrControl();
        SrcAddrControl getSrcAddrControl();
        bool doesRepeat();
        bool dmaTransferIs32Bit();
        bool gamepakDRQ();
        DMAStartTiming getStartTiming();
        bool irqOnEnd();
        bool isEnabled();
    };
}
