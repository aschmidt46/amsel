#pragma once
#include "../gba/scheduler.h"

namespace nds{
    class SharedBus;
    enum EventType{
        NDSEVENT_None,
        NDSEVENT_DmaTransfer7,
        NDSEVENT_DmaTransfer9,
        NDSEVENT_PPU_HBLANK,
        NDSEVENT_PPU_VBLANK,
        NDSEVENT_PPU_IncrementScanline,
        NDSEVENT_TimerOverflow7,
        NDSEVENT_TimerOverflow9,
        NDSEVENT_GameCardDataWord,
        NDSEVENT_GameCardChipID,
        NDSEVENT_SPITransferComplete
    };
}
