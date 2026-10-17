#pragma once
#include "../bus_types.h"



namespace nds{
    constexpr inline float scaleAxisComplex(float amin, float amax, float x, float goalMin, float goalMax){
        return (((x - amin) / (amax - amin)) * (goalMax - goalMin)) + goalMin;
    };

    union CONTROL_BYTE{
        struct{
            Byte powerDownModeSelect : 2;
            Byte refSelect : 1;
            Byte conversionMode : 1;
            Byte channelSelect : 3;
            Byte startBit : 1;
        } state;
        Byte raw;
    };

    class Bus7;

    class TouchController{
        bool penIRQ = true;
        bool vref = false;
        bool adc = false;
        
        HalfWord touchX12 = 0x0;
        HalfWord touchY12 = 0xFFF;
        HalfWord touchX8 = 0x0;
        HalfWord touchY8 = 0xFF;
        
        CONTROL_BYTE control = {.raw = 0};
        int bufferSize = 0;
        
        HalfWord readBuffer = 0;

        Bus7* bus = nullptr;
        
        
        public:
        void reset();
        TouchController(Bus7* bus) : bus(bus){};
        void setTouch(float  x, float y);
        void releaseTouch();

        void write(Byte val);
        Byte read();
    };
}
