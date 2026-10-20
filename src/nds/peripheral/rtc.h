#pragma once
#include "../bus_types.h"
#include <vector>


namespace nds{

    union RTC_STAT2_T {
        struct{
            Byte int1Mode : 4;
            Byte gp : 2;
            Byte int2Enable : 1;
            Byte tm : 1;
        } state;
        Byte raw;
    };

    union RTC_STAT1_T {
        struct{
            Byte reset : 1; //lsb
            Byte mode24h : 1;
            Byte gp : 2;
            Byte irq1 : 1;
            Byte irq2 : 1;
            Byte powLo : 1;
            Byte powOff : 1;
        } state;
        Byte raw;
    };
    constexpr Byte rtcStat1Mask = 0b00001110;

    union GPC_REG_T {
        struct{
            HalfWord dataio : 1; //lsb
            HalfWord clock : 1;
            HalfWord cs : 1;
            HalfWord _unused1 : 1;
            HalfWord dataDir : 1;
            HalfWord clockDir : 1;
            HalfWord SelDir : 1;
            HalfWord _rest : 9;
        } state;
        HalfWord raw;
    };


    struct GPIO{

        GPC_REG_T reg = {.raw = 0};

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

    class Bus7;

    // 30 Sekunden IRQ nicht implementiert
    class RTC : public GPIO{

        RTC_STAT1_T rtcStat1 = {.raw = 0};
        RTC_STAT2_T rtcStat2 = {.raw = 0};
        Bus7* bus;
        Byte clkAdjust = 0;
        Byte freereg = 0;

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
        RTC(Bus7* bus) : bus(bus){
            rtcStat1.state.mode24h = 1;
        };
        void onWrite(Word addr, Byte val) override;
        Byte onRead(Word addr) override;
    };
}
