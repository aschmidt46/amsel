#include "dma.h"
#include <bitset>
#include <iostream>
#include "bus.h"
#include "framework/stringlib.h"

using namespace gba;

constexpr std::array<Word, 4> srcMasks = {0x07FFFFFF, 0x0FFFFFFF, 0x0FFFFFFF, 0x0FFFFFFF};
constexpr std::array<Word, 4> destMasks = {0x07FFFFFF, 0x07FFFFFF, 0x07FFFFFF, 0x0FFFFFFF};
constexpr std::array<Word, 4> maxCountMasks = {0x3FFF, 0x3FFF, 0x3FFF, 0xFFFF};

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
            // remainingCycles += 2; // 2I, Achtung kann auch 4 sein (nicht implementiert)
            if(startTiming == DMA_IMMEDIATE){
                bus->scheduler->scheduleEvent({.timePoint = 3, .type = EVENT_DmaTransfer, .args={.index = this->dmaIndex}});
            }
        }
        else if(enabledBefore && !isEnabled()){
            // isActive = false;
        }
    }
}

void DMAChannel::resetInternalCounters(bool SAD, bool DAD){
    if(SAD) currentSourceAddr = SourceAddress.raw;
    if(DAD) currentDestAddr = DestinationAddress.raw;
    currentCount = 0;
    maxCount = WordCount.raw;

    currentSourceAddr &= srcMasks[dmaIndex];
    currentDestAddr &= destMasks[dmaIndex];

    if(dmaIndex == 3){ // any Memory
        maxCount &= 0xFFFF; // 16 bit
        if(maxCount == 0) maxCount = 0x10000;
    }
    else{ // internal memory
        maxCount &= 0x3FFF; // 14 bit
        if(maxCount == 0) maxCount = 0x4000;
    }
    if(startTiming == DMA_SOUND_FIFO)
        maxCount = 4;
    
    if(dmaTransferIs32Bit()){
        currentSourceAddr &= ~3u;
        currentDestAddr &= ~3u;
    }
    else{
        currentSourceAddr &= ~1u;
        currentDestAddr &= ~1u;
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
    // if(remainingCycles > 0){
    //     remainingCycles--;
    //     if(remainingCycles==0){
    //         // Am Ende
    //         if(currentCount >= maxCount){
    //             isActive = false; // Wird bei Reload wieder aktiv, wenn Bedingung eintritt
    //             if(doesRepeat()){
    //                 bool reloadDAD = getDestAddrControl() == DEST_INCREMENT_RELOAD;
    //                 resetInternalCounters(false, reloadDAD);
    //                 if(startTiming == DMA_IMMEDIATE) isActive = true;
    //             }
    //             else{
    //                 Control.raw &= ~(1u << 15);
    //             }

    //             if(irqOnEnd()){
    //                 bus->setIF(8 + dmaIndex, true);
    //             }
    //         }
    //     }
    //     return true;
    // }
    // else if(isEnabled() && isActive){
    //     if(currentCount == 0){
    //         remainingCycles += bus->getCyclesForAccess(currentSourceAddr, false);
    //         remainingCycles += bus->getCyclesForAccess(currentDestAddr, false);
    //     }
    //     else{
    //         remainingCycles += bus->getCyclesForAccess(currentSourceAddr, true);
    //         remainingCycles += bus->getCyclesForAccess(currentDestAddr, true);
    //     }

    //     if(dmaTransferIs32Bit()){
    //         const Word data = bus->readWord(currentSourceAddr);
    //         bus->writeWord(currentDestAddr, data);
    //     }
    //     else{
    //         const HalfWord data = bus->readHalfWord(currentSourceAddr);
    //         bus->writeHalfWord(currentDestAddr, data);
    //     }

    //     if(startTiming != DMA_SOUND_FIFO)
    //         currentDestAddr += destIncrement;
    //     currentSourceAddr += sourceIncrement;

    //     currentCount++;
    //     return true;
    // }
    // else{
    //     return false;
    // }
    return false;
}

void gba::DMAChannel::commenceTransfer()
{
    size_t remaining = 0;
    while(currentCount < maxCount){
        if(currentCount == 0){
            remaining += bus->getCyclesForAccess(currentSourceAddr, false);
            remaining += bus->getCyclesForAccess(currentDestAddr, false);
        }
        else{
            remaining += bus->getCyclesForAccess(currentSourceAddr, true);
            remaining += bus->getCyclesForAccess(currentDestAddr, true);
        }
        
        if(dmaTransferIs32Bit()){
            if(currentSourceAddr >= 0x2000000) // Dma open bus
                lastRead = bus->readWord(currentSourceAddr); // update latch
            const Word data = lastRead;
            bus->writeWord(currentDestAddr & ~3u, data);
        }
        else{
            if(currentSourceAddr >= 0x2000000){
                lastRead = bus->readHalfWord(currentSourceAddr);
                lastRead |= lastRead << 16; // Halbwort Wird in Latch dupliziert
            }
            Word data = lastRead;
            if(currentSourceAddr < 0x2000000 && currentDestAddr & 2)
                data >>= 16;
            bus->writeHalfWord(currentDestAddr & ~1u, data);
        }
    
        if(startTiming != DMA_SOUND_FIFO)
            currentDestAddr += destIncrement;
        else{
            bus->apu.clockFromDMA();
        }
        currentSourceAddr += sourceIncrement;

        currentSourceAddr &= srcMasks[dmaIndex];
        currentDestAddr &= destMasks[dmaIndex];
        
        currentCount++;
    }

    // Ende
    if(doesRepeat()){
        bool reloadDAD = getDestAddrControl() == DEST_INCREMENT_RELOAD;
        resetInternalCounters(false, reloadDAD);
        if(startTiming == DMA_IMMEDIATE){
            bus->scheduler->scheduleEvent({.timePoint = remaining + 3, .type = EVENT_DmaTransfer, .args={.index = this->dmaIndex}});
        }
    }
    else{
        Control.raw &= ~(1u << 15);
    }
    
    if(irqOnEnd()){
        bus->setIF(8 + dmaIndex, true);
    }

    bus->addCPUCycles(remaining);
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

