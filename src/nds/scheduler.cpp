#include "scheduler.h"
#include "bus.h"

using namespace nds;

// TIMINGS SIND FALSCH; ALLE ÜBERPRÜFEN

template<>
void gba::Scheduler<nds::EventType,nds::SharedBus>::handleEvent()
{
    auto &e = next.value();
    switch(e.type){
        case NDSEVENT_None:
            return;
        case NDSEVENT_DmaTransfer7:
            // bus->bus7->dma[e.args.index].commenceTransfer();
            return;
        case NDSEVENT_TimerOverflow7:
            // bus->bus7->timers[e.args.index].overflowTimer();
            return;
        case NDSEVENT_DmaTransfer9:
            // bus->bus9->dma[e.args.index].commenceTransfer();
            return;
        case NDSEVENT_TimerOverflow9:
            // bus->bus9->timers[e.args.index].overflowTimer();
            return;
        case NDSEVENT_PPU_HBLANK:
            bus->lcd->onHBlank();
            scheduleEvent({226, NDSEVENT_PPU_IncrementScanline});
            return;
        case NDSEVENT_PPU_VBLANK:
            bus->lcd->onVBlank();
            scheduleEvent({280896, NDSEVENT_PPU_VBLANK}); // 1232 cycles * 228 scanlines
            return;
        case NDSEVENT_PPU_IncrementScanline:
            bus->lcd->increment();
            if(bus->lcd->vCount < 160)
                bus->lcd->renderScanline();
            scheduleEvent({1006, NDSEVENT_PPU_HBLANK});
            return;
    }
    // std::cout << "dealt.\n";
}

template<>
void gba::Scheduler<nds::EventType, nds::SharedBus>::init()
{
    heap.push({1006, NDSEVENT_PPU_HBLANK});
    heap.push({197120, NDSEVENT_PPU_VBLANK});
}

template class gba::Scheduler<nds::EventType, nds::SharedBus>;
