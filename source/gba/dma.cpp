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
        bool enabledBefore = isEnabled();
        Control.OnWriteByte(addr, val);
        if(!enabledBefore && isEnabled()){
            std::cout << "Control write dma " << dmaIndex << "\n";
            if(getStartTiming() == DMA_SOUND_FIFO) std::cout << "Sound fifo\n";
            std::cout << std::bitset<16>(Control.raw) << "\n";
            std::cout << "src: " << getHex0x(SourceAddress.raw, 8) << ", dst: " << getHex0x(DestinationAddress.raw, 8) << ", count: " << WordCount.raw << "\n";
            resetInternalCounters(true, true);
            remainingCycles += 2; // 2I, Achtung kann auch 4 sein (nicht implementiert)
            if(getStartTiming() == DMA_IMMEDIATE) isActive = true;
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

DMAChannel::DMAChannel(int index, std::weak_ptr<Bus> busPtr) : bus(busPtr), dmaIndex(index), SourceAddress(0), DestinationAddress(4), WordCount(8), Control(10){}

bool DMAChannel::clock(){
    // muss ich noch implementieren, kann problematisch werden, wenn IO Register überschrieben werden
    if(getStartTiming() == DMA_SOUND_FIFO){
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
                    if(getStartTiming() == DMA_IMMEDIATE) isActive = true;
                }
                else{
                    Control.raw &= ~(1u << 15);
                }

                if(irqOnEnd()){
                    bus.lock()->setIF(8 + dmaIndex, true);
                }
            }
        }
        return true;
    }
    else if(isEnabled() && isActive){
        if(currentCount == 0){
            remainingCycles += bus.lock()->getCyclesForAccess(currentSourceAddr, false);
            remainingCycles += bus.lock()->getCyclesForAccess(currentDestAddr, false);
        }
        else{
            remainingCycles += bus.lock()->getCyclesForAccess(currentSourceAddr, true);
            remainingCycles += bus.lock()->getCyclesForAccess(currentDestAddr, true);
        }

        if(dmaTransferIs32Bit()){
            const Word data = bus.lock()->readWord(currentSourceAddr);
            bus.lock()->writeWord(currentDestAddr, data);
        }
        else{
            const HalfWord data = bus.lock()->readHalfWord(currentSourceAddr);
            bus.lock()->writeHalfWord(currentDestAddr, data);
        }

        const int incrementer = dmaTransferIs32Bit() ? 4 : 2;

        switch(getDestAddrControl()){
            case DEST_INCREMENT:
            case DEST_INCREMENT_RELOAD:
                currentDestAddr += incrementer;
                break;
            case DEST_DESCREMENT:
                currentDestAddr -= incrementer;
                break;
            default:
                break;
        }

        switch(getSrcAddrControl()){
            case SRC_INCREMENT:
                currentDestAddr += incrementer;
                break;
            case SRC_DESCREMENT:
                currentDestAddr -= incrementer;
                break;
            default:
                break;
        }

        currentCount++;
        return true;
    }
    else{
        return false;
    }
}

DestAddrControl DMAChannel::getDestAddrControl(){
    return DestAddrControl((Control.raw >> 5) & 0b11u);
}

SrcAddrControl DMAChannel::getSrcAddrControl(){
    return SrcAddrControl((Control.raw >> 7) & 0b11u);
}

bool DMAChannel::doesRepeat(){
    return Control.raw & (1u << 9);
}

bool DMAChannel::dmaTransferIs32Bit(){
    return Control.raw & (1u << 10);
}

bool DMAChannel::gamepakDRQ(){
    return Control.raw & (1u << 11);
}

DMAStartTiming DMAChannel::getStartTiming(){
    DMAStartTiming timing = DMAStartTiming((Control.raw >> 12) & 0b11u);
    if((dmaIndex == 1 || dmaIndex == 2) && timing == DMA_SPECIAL_PROHIBITED){
        timing = DMA_SOUND_FIFO;
    }
    else if(dmaIndex == 3 && timing == DMA_SPECIAL_PROHIBITED){
        timing = DMA_VIDEO_CAPTURE;
    }
    return timing;
}

bool DMAChannel::irqOnEnd(){
    return Control.raw & (1u << 14);
}

bool DMAChannel::isEnabled(){
    return Control.raw & (1u << 15);
}

