#pragma once
#include <queue>
#include <optional>

namespace gba{
    class Bus;

    enum EventType{
        EVENT_None,
        EVENT_DmaTransfer,
        EVENT_TimerIncrement,
        EVENT_APU_PSG,
        EVENT_APU_LengthCounters,
        EVENT_APU_Sweep,
        EVENT_APU_Envelopes,
        EVENT_PPU_DrawScanline,
        EVENT_PPU_HBLANK,
        EVENT_PPU_VBLANK,
        EVENT_PPU_IncrementScanline,
    };

    struct Event{
        size_t timePoint;
        EventType type;

        union{

            int index;

        } args;

        auto operator<=>(const Event &b) const{
            return b.timePoint <=> timePoint;
        };
    };

    class Scheduler{
        std::priority_queue<Event> heap;
        size_t time = 0;
        std::optional<Event> next = {};
        Bus* bus;

        void handleEvent();

        public:
        Scheduler(Bus* bus) : bus(bus){};
        void init();
        bool cpuBlocked = false;
        void scheduleEvent(Event e);
        void clock();
    };
}