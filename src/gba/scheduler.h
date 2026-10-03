#pragma once
#include <queue>
#include <optional>
#include "arm/bus_types.h"

namespace gba{
    class Bus;

    enum EventType{
        EVENT_None,
        EVENT_DmaTransfer,
        EVENT_APU_PSG,
        EVENT_APU_LengthCounters,
        EVENT_APU_Sweep,
        EVENT_APU_Envelopes,
        EVENT_PPU_HBLANK,
        EVENT_PPU_VBLANK,
        EVENT_PPU_IncrementScanline,
        EVENT_TimerOverflow,
    };

    template<typename T>
    struct Event{
        size_t timePoint;
        T type;

        union{

            int index;

        } args;

        auto operator<=>(const Event<T> &b) const{
            return b.timePoint <=> timePoint;
        };
    };

    template<typename T, typename B>
    class Scheduler{
        std::priority_queue<Event<T>> heap;
        size_t time = 0;
        std::optional<Event<T>> next = {};
        B* bus;

        void handleEvent();

        public:
        Scheduler(B* bus) : bus(bus){};
        void init();
        bool cpuBlocked = false;
        void scheduleEvent(Event<T> e);
        void clock();
    };
}