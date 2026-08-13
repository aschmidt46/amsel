#include "dma.h"
#include <bitset>
#include <iostream>
#include "bus.h"
#include "framework/stringlib.h"

using namespace gba;

void DMAChannel::onWrite(Word addr, Byte val){
    if(addr < 4){
        SourceAddress.OnWriteByte(addr, val);
    }
    else if(addr < 8){
        DestinationAddress.OnWriteByte(addr, val);
    }
    else if(addr < 10){
        WordCount.OnWriteByte(addr, val);
    }
    else if(addr < 12){
        const bool enabledBefore = isEnabled();
        Control.OnWriteByte(addr, val);
        startTiming = getStartTiming();
        const int incrementer = dmaTransferIs32Bit() ? 4 : 2;
        int destIncrementFactor = 0;
        switch(getDestAddrControl()){
            case DEST_INCREMENT:
            case DEST_INCREMENT_RELOAD:
                destIncrementFactor = 1;
                break;
            case DEST_DESCREMENT:
                destIncrementFactor = -1;
                break;
            default:
                break;
        }
        int sourceIncrementFactor = 0;
        switch(getSrcAddrControl()){
            case SRC_INCREMENT:
                sourceIncrementFactor = 1;
                break;
            case SRC_DESCREMENT:
                sourceIncrementFactor = -1;
                break;
            default:
                break;
        }
        sourceIncrement = sourceIncrementFactor * incrementer;
        destIncrement = destIncrementFactor * incrementer;


        if(!enabledBefore && isEnabled()){
            // std::cout << "Control write dma " << dmaIndex << "\n";
            // std::cout << std::bitset<16>(Control.raw) << "\n";
            // printStartTiming();
            // std::cout << "src: " << getHex0x(SourceAddress.raw, 8) << ", dst: " << getHex0x(DestinationAddress.raw, 8) << ", count: " << WordCount.raw << "\n";
            resetInternalCounters(true, true);
            remainingCycles += 2; // 2I, Achtung kann auch 4 sein (nicht implementiert)
            if(startTiming == DMA_IMMEDIATE) isActive = true;
        }
        else if(enabledBefore && !isEnabled()){
            isActive = false;
        }
    }
}

void DMAChannel::resetInternalCounters(bool SAD, bool DAD){
    if(SAD) currentSourceAddr = SourceAddress.raw;
    if(DAD) currentDestAddr = DestinationAddress.raw;
    currentCount = 0;
    maxCount = WordCount.raw;

    if(dmaIndex == 0){ // internal Memory
        currentSourceAddr &= 0x7FFFFFF;
    }
    else{ // any memory
        currentSourceAddr &= 0xFFFFFFF;
    }

    if(dmaIndex == 3){ // any Memory
        currentDestAddr &= 0xFFFFFFF;
        maxCount &= 0xFFFF; // 16 bit
        if(maxCount == 0) maxCount = 0x10000;
    }
    else{ // internal memory
        currentDestAddr &= 0x7FFFFFF;
        maxCount &= 0x3FFF; // 14 bit
        if(maxCount == 0) maxCount = 0x4000;
    }
}

Byte DMAChannel::onRead(Word addr){
    if(addr >= 10 && addr < 12){
        return Control.OnReadByte(addr);
    }
    else{
        return 0;
    }
}

DMAChannel::DMAChannel(int index, Bus* busPtr) : bus(busPtr), dmaIndex(index), SourceAddress(0), DestinationAddress(4), WordCount(8), Control(10){}

bool DMAChannel::clock(){
    // muss ich noch implementieren, kann problematisch werden, wenn IO Register überschrieben werden
    if(startTiming == DMA_SOUND_FIFO){
        isActive = false;
        Control.raw &= ~(1u << 15);
        return false;
    }
    if(remainingCycles > 0){
        remainingCycles--;
        if(remainingCycles==0){
            // Am Ende
            if(currentCount >= maxCount){
                isActive = false; // Wird bei Reload wieder aktiv, wenn Bedingung eintritt
                if(doesRepeat()){
                    bool reloadDAD = getDestAddrControl() == DEST_INCREMENT_RELOAD;
                    resetInternalCounters(false, reloadDAD);
                    if(startTiming == DMA_IMMEDIATE) isActive = true;
                }
                else{
                    Control.raw &= ~(1u << 15);
                }

                if(irqOnEnd()){
                    bus->setIF(8 + dmaIndex, true);
                }
            }
        }
        return true;
    }
    else if(isEnabled() && isActive){
        if(currentCount == 0){
            remainingCycles += bus->getCyclesForAccess(currentSourceAddr, false);
            remainingCycles += bus->getCyclesForAccess(currentDestAddr, false);
        }
        else{
            remainingCycles += bus->getCyclesForAccess(currentSourceAddr, true);
            remainingCycles += bus->getCyclesForAccess(currentDestAddr, true);
        }

        if(dmaTransferIs32Bit()){
            const Word data = bus->readWord(currentSourceAddr);
            bus->writeWord(currentDestAddr, data);
        }
        else{
            const HalfWord data = bus->readHalfWord(currentSourceAddr);
            bus->writeHalfWord(currentDestAddr, data);
        }

        currentDestAddr += destIncrement;
        currentSourceAddr += sourceIncrement;

        currentCount++;
        return true;
    }
    else{
        return false;
    }
}

void DMAChannel::printStartTiming(){
    auto s = getStartTiming();
    switch(s){
        case DMA_VIDEO_CAPTURE:
            std::cout << "Video Capture\n";
            break;
        case DMA_SOUND_FIFO:
            std::cout << "Sound FIFO\n";
            break;
        case DMA_IMMEDIATE:
            std::cout << "Immediate\n";
            break;
        case DMA_HBLANK:
            std::cout << "HBLANK\n";
            break;
        case DMA_VBLANK:
            std::cout << "VBLANK\n";
            break;
        case DMA_SPECIAL_PROHIBITED:
            std::cout << "Dma verboten\n";
            break;
    }
}

