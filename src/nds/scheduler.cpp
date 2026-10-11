#include "scheduler.h"
#include "bus.h"
#include <iostream>

using namespace nds;

template<>
void gba::Scheduler<nds::EventType, nds::SharedBus>::scheduleEvent(Event<nds::EventType> e)
{
    e.timePoint += time;
    heap.push(e);
}

template<>
void gba::Scheduler<nds::EventType,nds::SharedBus>::handleEvent()
{
    auto &e = next.value();
    switch(e.type){
        case NDSEVENT_None:
            return;
        case NDSEVENT_DmaTransfer7:
            bus->bus7->dma[e.args.index].commenceTransfer();
            return;
        case NDSEVENT_TimerOverflow7:
            bus->bus7->timers[e.args.index].overflowTimer();
            return;
        case NDSEVENT_DmaTransfer9:
            bus->bus9->dma[e.args.index].commenceTransfer();
            return;
        case NDSEVENT_TimerOverflow9:
            bus->bus9->timers[e.args.index].overflowTimer();
            return;
        case NDSEVENT_PPU_HBLANK:
            bus->lcd->onHBlank();
            scheduleEvent({2130, NDSEVENT_PPU_IncrementScanline});
            return;
        case NDSEVENT_PPU_VBLANK:
            bus->lcd->onVBlank();
            // std::cout << "lcd blank\n";
            scheduleEvent({560190, NDSEVENT_PPU_VBLANK}); // 1232 cycles * 228 scanlines
            return;
        case NDSEVENT_PPU_IncrementScanline:
            bus->lcd->increment();
            if(bus->lcd->vCount < 192) //nee
                bus->lcd->renderScanline();
            scheduleEvent({1536, NDSEVENT_PPU_HBLANK});
            return;
    }
    // std::cout << "dealt.\n";
}

template<>
void gba::Scheduler<nds::EventType, nds::SharedBus>::init()
{
    heap.push({1536, NDSEVENT_PPU_HBLANK});
    heap.push({545280, NDSEVENT_PPU_VBLANK});
}

template<>
void gba::Scheduler<nds::EventType, nds::SharedBus>::clock()
{
    bool tryAgain = false;
    do{
        tryAgain = false;
        if(!heap.empty())
            next = heap.top();
        if(next.has_value() && time >= next.value().timePoint){
            handleEvent();
            heap.pop();
            next = {};
            tryAgain = true;
        }
        if(!next.has_value() && !heap.empty()){
            next = heap.top();
        }
    } while (tryAgain);
    time++;
}

template class gba::Scheduler<nds::EventType, nds::SharedBus>;
