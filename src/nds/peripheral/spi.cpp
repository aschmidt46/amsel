#include "spi.h"
#include "../bus7.h"
#include "../bus.h"
#include <iostream>

using namespace nds;

void nds::SPI::startTransfer()
{
    SPICNT.state.busyFlag = 1;
    if(SPICNT.state.deviceSelect == 1){
        firmware.write(SPIDATA);
    }
    bus->sharedBus->scheduler->scheduleEvent({.timePoint= 10, .type= NDSEVENT_SPITransferComplete});
}

void nds::SPI::onWrite(Word addr, Byte val)
{
    if(addr == 0x040001C0){
        SPICNT.raw = (SPICNT.raw & 0xFF00) | (val & 0b00000011);
    }
    else if(addr == 0x040001C1){
        SPICNT.raw = (SPICNT.raw & 0x00FF) | (HalfWord(val & 0b11001111) << 8);
    }
    else if(addr == 0x040001C2){
        SPIDATA = val;
        if(SPICNT.state.busyFlag == 0){
            startTransfer();
        }
    }
}

Byte nds::SPI::onRead(Word addr)
{
    if(addr == 0x040001C0){
        return SPICNT.raw;
    }
    if(addr == 0x040001C1){
        return SPICNT.raw >> 8;
    }
    if(addr == 0x040001C2){
        if(!SPICNT.state.chipSelectHold){
            if(SPICNT.state.deviceSelect == 1){
                firmware.state = FIRMWARE_IDLE;
            }
        }
        const auto tmp = SPIDATA;
        SPIDATA = 0;
        return tmp;
    }
    return 0;
}

void nds::SPI::completeTransfer()
{
    SPICNT.state.busyFlag = 0;
    if(SPICNT.state.interruptRequest){
        bus->setIF(23, true);
    }
    
    if(SPICNT.state.deviceSelect == 1) SPIDATA = firmware.read();

    if(SPICNT.state.transferSize){ // jedes zweite Byte wird verworfen
        if(SPICNT.state.deviceSelect == 1) (void)firmware.read();
    }
}

void nds::SPI::setFirmware(const std::vector<Byte> &data)
{
    firmware.firmwareData = data;
}
