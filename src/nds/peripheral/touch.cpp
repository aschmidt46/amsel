#include "touch.h"
#include "../bus7.h"
#include "../bus.h"
#include <iostream>
#include "framework/stringlib.h"

using namespace nds;

void nds::TouchController::reset()
{
    bufferSize = 0;
}

void nds::TouchController::setTouch(float x, float y)
{
    touchX12 = (int)scaleAxisComplex(0, 256, x, 0x100, 0xED0);
    touchY12 = (int)scaleAxisComplex(0, 192, y, 0xB0, 0xF20);

    touchX8 = (float(touchX12) / float(0x1000)) * 0xFF;
    touchY8 = (float(touchY12) / float(0x1000)) * 0xFF;
    // if(penIRQ){
        bus->sharedBus->EXKEYINPUT &= ~(1u << 6);
    // }
}

void nds::TouchController::releaseTouch()
{
    touchX12 = 0x0;
    touchY12 = 0xFFF;
    touchX8 = 0x0;
    touchY8 = 0xFF;

    // if(penIRQ){
        bus->sharedBus->EXKEYINPUT |= (1u << 6);
    // }
}

void nds::TouchController::write(Byte val)
{
    if(((val >> 7) & 1u) == 0) return;

    bufferSize = 2;



    std::cout << "touch write\n";
    control.raw = val;
    switch(control.state.powerDownModeSelect){
        case 0:
            penIRQ = true;
            vref = true;
            adc = true;
            break;
        case 1:
            penIRQ = false;
            vref = false;
            adc = true;
            break;
        case 2:
            penIRQ = true;
            vref = true;
            adc = false;
            break;
        default:
            penIRQ = false;
            vref = true;
            adc = true;
    }
    // if(!penIRQ){
    //     bus->sharedBus->EXKEYINPUT |= (1u << 6);
    // }

    switch(control.state.channelSelect){
        case 1:{
            if(control.state.conversionMode > 0){
                readBuffer = HalfWord(touchY8) << 1;
            }
            else{
                readBuffer = HalfWord(touchY12) << 1;
            }
            break;
        }
        case 5:{
            if(control.state.conversionMode > 0){
                readBuffer = HalfWord(touchX8) << 1;
            }
            else{
                readBuffer = HalfWord(touchX12) << 1;
            }
            break;
        }
    }
}

Byte nds::TouchController::read()
{
    const Byte tmp = readBuffer;
    readBuffer >>= 8;
    bufferSize--;
    std::cout << "read: " << getHex0x(tmp, 2) << "\n";
    return tmp;
}
