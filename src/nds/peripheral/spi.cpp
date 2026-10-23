#include "spi.h"
#include "../bus7.h"
#include "../bus.h"
#include <iostream>

using namespace nds;

void nds::SPI::startTransfer()
{
    SPICNT.state.busyFlag = 1;
    switch(SPICNT.state.deviceSelect){
        case 0:
            power.write(SPIDATA);
            break;
        case 1:
            firmware.write(SPIDATA);
            break;
        case 2:
            touch.write(SPIDATA);
            break;
        default: break;
    }
    // std::cout << "Transfer on spi device " << SPICNT.state.deviceSelect << "\n";
    bus->sharedBus->scheduler->scheduleEvent({.timePoint= 10, .type= NDSEVENT_SPITransferComplete});
}

void nds::SPI::unselectDevices()
{
    if(firmware.state == FIRMWARE_PW){
        firmware.status &= ~2u;
        firmware.awaitRead = false;
    };
    if(firmware.state != FIRMWARE_DP)
        firmware.state = FIRMWARE_IDLE;
    power.awaitRead = false;
    power.indexSelected = false;
    touch.reset();
}

void nds::SPI::onWrite(Word addr, Byte val)
{
    if(addr == 0x040001C0){
        SPICNT.raw = (SPICNT.raw & 0xFF00) | (val & 0b00000011);
    }
    else if(addr == 0x040001C1){
        SPICNT.raw = (SPICNT.raw & 0x00FF) | (HalfWord(val & 0b11001111) << 8);
        // std::cout << "SPICNT device select: " << SPICNT.state.deviceSelect << "\n";
        if(SPICNT.state.spiBusEnable == 0){
            unselectDevices();
        }
    }
    else if(addr == 0x040001C2){
        SPIDATA = val;
        if(SPICNT.state.busyFlag == 0 && SPICNT.state.spiBusEnable){
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
        return SPIDATA;
    }
    return 0;
}

void nds::SPI::completeTransfer()
{
    SPICNT.state.busyFlag = 0;
    if(SPICNT.state.interruptRequest){
        bus->setIF(23, true);
    }

    switch(SPICNT.state.deviceSelect){
        case 0:
            SPIDATA = power.read();
            break;
        case 1:
            SPIDATA = firmware.read();
            break;
        case 2:
            SPIDATA = touch.read();
            break;
        default: break;
    }

    if(SPICNT.state.transferSize){ // jedes zweite Byte wird verworfen
        switch(SPICNT.state.deviceSelect){
            case 0:
                (void)power.read();
                break;
            case 1:
                (void)firmware.read();
                break;
            case 2:
                (void)touch.read();
                break;
            default: break;
        }
    }
    if(!SPICNT.state.chipSelectHold) unselectDevices();
}

void nds::SPI::setFirmware(const std::vector<Byte> &data)
{
    firmware.firmwareData = data;
}
