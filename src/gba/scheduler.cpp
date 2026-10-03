#include "scheduler.h"
#include "bus.h"
#include <iostream>
#include <limits>

template<>
void gba::Scheduler<gba::EventType,gba::Bus>::handleEvent()
{
    auto &e = next.value();
    switch(e.type){
        case EVENT_None:
            return;
        case EVENT_DmaTransfer:
            bus->dma[e.args.index].commenceTransfer();
            return;
        case EVENT_TimerOverflow:
            bus->timers[e.args.index].overflowTimer();
            return;
        case EVENT_APU_PSG:
            bus->apu.clockPSG();
            scheduleEvent({4, EVENT_APU_PSG});
            return;
        case EVENT_APU_LengthCounters:
            bus->apu.clockLengthCounters();
            scheduleEvent({0x10000, EVENT_APU_LengthCounters});
            return;
        case EVENT_APU_Sweep:
            bus->apu.clockSweep();
            scheduleEvent({0x20000, EVENT_APU_Sweep});
            return;
        case EVENT_APU_Envelopes:
            bus->apu.clockEnvelopes();
            scheduleEvent({0x40000, EVENT_APU_Envelopes});
            return;
        case EVENT_PPU_HBLANK:
            bus->ppu.onHBlank();
            scheduleEvent({226, EVENT_PPU_IncrementScanline});
            return;
        case EVENT_PPU_VBLANK:
            bus->ppu.onVBlank();
            scheduleEvent({280896, EVENT_PPU_VBLANK}); // 1232 cycles * 228 scanlines
            return;
        case EVENT_PPU_IncrementScanline:
            bus->ppu.increment();
            if(bus->ppu.currentScanline < 160)
                bus->drawScanline();
            scheduleEvent({1006, EVENT_PPU_HBLANK});
            return;
    }
    // std::cout << "dealt.\n";
}

template<>
void gba::Scheduler<gba::EventType, gba::Bus>::init()
{
    heap.push({4, EVENT_APU_PSG});
    heap.push({0x10000, EVENT_APU_LengthCounters});
    heap.push({0x20000, EVENT_APU_Sweep});
    heap.push({0x40000, EVENT_APU_Envelopes});
    heap.push({1006, EVENT_PPU_HBLANK});
    heap.push({197120, EVENT_PPU_VBLANK});
}

template<typename T, typename B>
void gba::Scheduler<T,B>::scheduleEvent(Event<T> e)
{
    e.timePoint += time;
    heap.push(e);
    // std::cout << "scheduled\n";
}

template<typename T, typename B>
void gba::Scheduler<T,B>::clock()
{
    cpuBlocked = false;
    bool tryAgain = false;
    do{
        tryAgain = false;
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

template class gba::Scheduler<gba::EventType, gba::Bus>;
