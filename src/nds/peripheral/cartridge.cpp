#include "cartridge.h"
#include <iostream>
#include "framework/stringlib.h"
#include "../bus.h"

using namespace nds;

void nds::Cartridge::startTransfer()
{
    switch(getCMD()){
        case 0xB7:{
            std::cout << "Gamecard Read Command\n";
            // std::cout << getHex0x(gameCardBusCommandOut, 16) << "\n";
            // std::cout << romControl.getTranferSize() << "\n";
            romControl.state.dataWordStatus = 0;
            remainingLength = romControl.getTranferSize();
            offsetAddress = (gameCardBusCommandOut >> 24) & 0xFFFFFFFF;
            currentMode = NDSEVENT_GameCardDataWord;
            if(offsetAddress < 0x8000){
                offsetAddress = 0x8000 + (offsetAddress & 0x1FF);
            }
            std::cout << getHex0x(offsetAddress, 8) << "\n";
            oncePerCycle = false;
            if(remainingLength > 0){
                bus->scheduler->scheduleEvent({.timePoint = 80 + romControl.state.transferCLKRate ? 8u : 5u, .type = NDSEVENT_GameCardDataWord});
            }
            else{
                checkAndResend();
            }
            break;
        }
        case 0xB8:{
            std::cout << "CHIP ID Command\n";
            romControl.state.dataWordStatus = 0;
            remainingLength = romControl.getTranferSize();
            currentMode = NDSEVENT_GameCardChipID;
            oncePerCycle = false;
            if(remainingLength > 0){
                bus->scheduler->scheduleEvent({.timePoint = 80 + romControl.state.transferCLKRate ? 8u : 5u, .type = NDSEVENT_GameCardChipID});
            }
            else{
                checkAndResend();
            }
            break;
        }
        default:{
            std::cout << "Unknown Gamecard Command: " << getHex0x(getCMD(), 2) << "\n";
        }
    }
}

nds::Cartridge::Cartridge(SharedBus *bus, const std::vector<Byte> &cart) : bus(bus), cartridge(cart)
{
}

Byte nds::Cartridge::onRead(Word addr)
{
    if(addr == 0x40001A0){
        return AUXSPICNT;
    }
    if(addr == 0x40001A1){
        return AUXSPICNT >> 8;
    }
    if(addr == 0x40001A2){
        return AUXSPIDATA;
    }
    if(addr == 0x40001A4){
        return romControl.raw;
    }
    if(addr == 0x40001A5){
        return romControl.raw >> 8;
    }
    if(addr == 0x40001A6){
        return romControl.raw >> 16;
    }
    if(addr == 0x40001A7){
        return romControl.raw >> 24;
    }
    if(addr == 0x4100010){
        romControl.state.dataWordStatus = 0;
        checkAndResend();
        return gameCardBus4byteDataIn;
    }
    if(addr == 0x4100011){
        romControl.state.dataWordStatus = 0;
        checkAndResend();
        return gameCardBus4byteDataIn >> 8;
    }
    if(addr == 0x4100012){
        romControl.state.dataWordStatus = 0;
        checkAndResend();
        return gameCardBus4byteDataIn >> 16;
    }
    if(addr == 0x4100013){
        romControl.state.dataWordStatus = 0;
        checkAndResend();
        return gameCardBus4byteDataIn >> 24;
    }
    return 0;
}


void nds::Cartridge::onWrite(Word addr, Byte val)
{
    if(addr == 0x40001A0){
        AUXSPICNT = (AUXSPICNT & 0xFF00) | (val & 0b01000011);
    }
    else if(addr == 0x40001A1){
        AUXSPICNT = (AUXSPICNT & 0x00FF) | (HalfWord(val & 0b11100000) << 8);
    }
    else if(addr == 0x40001A2){
        AUXSPIDATA = val;
    }
    else if(addr == 0x40001A4){
        romControl.raw = (romControl.raw & 0xFFFFFF00) | (val);
    }
    else if(addr == 0x40001A5){
        romControl.raw = (romControl.raw & 0xFFFF00FF) | (HalfWord(val) << 8);
    }
    else if(addr == 0x40001A6){
        romControl.raw = (romControl.raw & 0xFF00FFFF) | (HalfWord(val & 127u) << 16);
    }
    else if(addr == 0x40001A7){
        bool before = romControl.state.blockStartStatus > 0;
        romControl.raw = (romControl.raw & 0x00FFFFFF) | (HalfWord(val) << 24);
        bool after = romControl.state.blockStartStatus > 0;
        if(!before && after){
            startTransfer();
        }
    }
    else if(addr >= 0x40001A8 && addr < 0x40001B0){
        addr -= 0x40001A8;
        // std::cout << "ROMCMD write byte:" << addr << " sz:1 " << "data:" << getHex0x(val,2)<<"\n";
        if(addr == 0){
            gameCardBusCommandOut = (gameCardBusCommandOut & 0x00FFFFFFFFFFFFFF) | (uint64_t(val) << 56);
        }
        else if(addr == 1){
            gameCardBusCommandOut = (gameCardBusCommandOut & 0xFF00FFFFFFFFFFFF) | (uint64_t(val) << 48);
        }
        else if(addr == 2){
            gameCardBusCommandOut = (gameCardBusCommandOut & 0xFFFF00FFFFFFFFFF) | (uint64_t(val) << 40);
        }
        else if(addr == 3){
            gameCardBusCommandOut = (gameCardBusCommandOut & 0xFFFFFF00FFFFFFFF) | (uint64_t(val) << 32);
        }
        else if(addr == 4){
            gameCardBusCommandOut = (gameCardBusCommandOut & 0xFFFFFFFF00FFFFFF) | (uint64_t(val) << 24);
        }
        else if(addr == 5){
            gameCardBusCommandOut = (gameCardBusCommandOut & 0xFFFFFFFFFF00FFFF) | (uint64_t(val) << 16);
        }
        else if(addr == 6){
            gameCardBusCommandOut = (gameCardBusCommandOut & 0xFFFFFFFFFFFF00FF) | (uint64_t(val) << 8);
        }
        else if(addr == 7){
            gameCardBusCommandOut = (gameCardBusCommandOut & 0xFFFFFFFFFFFFFF00) | (uint64_t(val));
        }
        // gameCardBusCommandOut = (gameCardBusCommandOut & ~(uint64_t(0xFF) << (56 - addr * 8))) | (uint64_t(val) << (56 - addr * 8));
    }
}

void nds::Cartridge::sendDataWord()
{
    gameCardBus4byteDataIn = cartridge[offsetAddress] | (Word(cartridge[offsetAddress + 1]) << 8) | (Word(cartridge[offsetAddress + 2]) << 16) | (Word(cartridge[offsetAddress + 3]) << 16);
    offsetAddress += 4;
    remainingLength -= 4;

    romControl.state.dataWordStatus = 1;
    oncePerCycle = false;
}

void nds::Cartridge::sendChipID()
{
    gameCardBus4byteDataIn = bus->chipID;
    romControl.state.dataWordStatus = 1;
    remainingLength -= 4;
    oncePerCycle = false;
}

void nds::Cartridge::checkAndResend()
{
    if(oncePerCycle) return;
    oncePerCycle = true;
    if(remainingLength > 0){
        bus->scheduler->scheduleEvent({.timePoint = romControl.state.transferCLKRate ? 8u : 5u, .type = currentMode});
    }
    else{
        romControl.state.blockStartStatus = 0;
        romControl.state.dataWordStatus = 0;

        if(((AUXSPICNT >> 14) & 1u) > 0){
            if(bus->canAccessGameCart(false)){
                bus->setIFSingleBus(19, true, false);
            }
            else{
                bus->setIFSingleBus(19, true, true);
            }
        }
    }
}
