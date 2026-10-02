#pragma once
#include "../arm/bus_types.h"
#include <vector>


namespace gba{

    union RTC_CONTROL_T {
        struct{
            Byte zero0 : 1; //lsb
            Byte _fill0 : 1;
            Byte _zero1 : 1;
            Byte perMinIRQ : 1;
            Byte _zero2 : 2;
            Byte mode24h : 1;
            Byte powerOff : 1;
        } state;
        Byte raw;
    };
    constexpr Byte rtcControlMask = 0b11001010; // 0 bits müssen immer 0 sein


    struct GPIO{
        Byte direction = 0;
        Byte data = 0;
        Byte control = 0;

        virtual void onWrite(Word addr, Byte val);
        virtual Byte onRead(Word addr);
        virtual ~GPIO(){};
    };

    enum RTCMode{
        RTC_IDLE,
        RTC_CMD,
        RTC_CMD_READ,
        RTC_CMD_WRITE,
    };

    class Bus;

    // 30 Sekunden IRQ nicht implementiert
    class RTC : public GPIO{

        RTC_CONTROL_T rtcControl = {.raw = 0};
        Bus* bus;

        std::vector<Byte> registerQueue = {};
        Byte readCommandLatch = 0;
        size_t bitsRead = 0;
        size_t bytesRead = 0;

        bool SCK = false;
        bool SIO = false;
        bool CS = false;

        bool reverseBitOrder = false;
        Word registerIndexSelected = 0;

        RTCMode mode = RTC_IDLE;

        void putOnReadQueue();


        public:
        RTC(Bus* bus) : bus(bus){
            rtcControl.state.mode24h = 1;
        };
        void onWrite(Word addr, Byte val) override;
        Byte onRead(Word addr) override;
    };
}
