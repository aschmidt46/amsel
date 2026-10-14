#include "dma.h"
#include <bitset>
#include <iostream>
#include "../bus.h"
#include "framework/stringlib.h"

using namespace nds;

constexpr std::array<Word, 4> srcMasks = {0x07FFFFFF, 0x0FFFFFFF, 0x0FFFFFFF, 0x0FFFFFFF};
constexpr std::array<Word, 4> destMasks = {0x07FFFFFF, 0x07FFFFFF, 0x07FFFFFF, 0x0FFFFFFF};
// constexpr std::array<Word, 4> maxCountMasks = {0x3FFF, 0x3FFF, 0x3FFF, 0xFFFF};

template<typename ConcreteBus>
void DMAChannel<ConcreteBus>::onWrite(Word addr, Byte val){
    // std::cout << "DMA Bus " << (isDMA7() ? "7" : "9") << " write, addr: " << addr << "\n";
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
        // std::cout << startTiming << "\n";
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
            if(SourceAddress.raw == 0x08000080) bus->setHalt();
            std::cout << "SETTING DMA Transfer Index " << dmaIndex <<  " on Bus " << (isDMA7() ? "7" : "9") << " with Timing ";
            printStartTiming();
            std::cout << "\n";
            std::cout << "Source Addr " << getHex0x(SourceAddress.raw, 8) << ", Dest Addr " << getHex0x(DestinationAddress.raw, 8) << ", Size " << getHex0x(WordCount.raw, 8) << "\n";
            resetInternalCounters(true, true);
            if(startTiming == DMA_IMMEDIATE){
                bus->scheduler->scheduleEvent({.timePoint = 3, .type = schedulerEvent, .args={.index = this->dmaIndex}});
            }
        }
        else if(enabledBefore && !isEnabled()){
            // isActive = false;
        }
    }
}

template<typename ConcreteBus>
void DMAChannel<ConcreteBus>::resetInternalCounters(bool SAD, bool DAD){
    if(SAD) currentSourceAddr = SourceAddress.raw;
    if(DAD) currentDestAddr = DestinationAddress.raw;
    currentCount = 0;
    maxCount = WordCount.raw;

    currentSourceAddr &= isDMA7() ? srcMasks[dmaIndex] : 0xFFFFFFE;
    currentDestAddr &= isDMA7() ? destMasks[dmaIndex] : 0xFFFFFFE;

    if(dmaIndex == 3){ // any Memory
        maxCount &= isDMA7() ? 0xFFFF : 0x1FFFFF;
        if(maxCount == 0) maxCount = isDMA7() ? 0x10000 : 0x200000;
    }
    else{ // internal memory
        maxCount &= isDMA7() ? 0x3FFF : 0x1FFFFF; // 14 bit
        if(maxCount == 0) maxCount = isDMA7() ? 0x4000 : 0x200000;
    }
    
    if(dmaTransferIs32Bit()){
        currentSourceAddr &= ~3u;
        currentDestAddr &= ~3u;
    }
    else{
        currentSourceAddr &= ~1u;
        currentDestAddr &= ~1u;
    }
}

template<typename ConcreteBus>
Byte DMAChannel<ConcreteBus>::onRead(Word addr){
    if(addr < 4){
        return SourceAddress.OnReadByte(addr);
    }
    else if(addr < 8){
        return DestinationAddress.OnReadByte(addr);
    }
    else if(addr < 10){
        return WordCount.OnReadByte(addr);
    }
    else if(addr < 12){
        return Control.OnReadByte(addr);
    }
    else{
        return 0;
    }
}

template<typename ConcreteBus>
DMAChannel<ConcreteBus>::DMAChannel(int index, SharedBus *busPtr, ConcreteBus *cbus, EventType event)
    : bus(busPtr), cbus(cbus), dmaIndex(index), schedulerEvent(event), SourceAddress(0), DestinationAddress(4), WordCount(8), Control(10){}


template<typename ConcreteBus>
void DMAChannel<ConcreteBus>::commenceTransfer()
{
    std::cout << "Starting DMA Transfer on Bus " << (isDMA7() ? "7" : "9") << " with Timing ";
    printStartTiming(); 
    size_t remaining = 0;
    while(currentCount < maxCount){
        remaining += 2;
        
        
        if(dmaTransferIs32Bit()){
            if(currentSourceAddr >= 0x2000000) // Dma open bus
                lastRead = cbus->readWordFromDMA(currentSourceAddr); // update latch
            const Word data = lastRead;
            cbus->writeWordFromDMA(currentDestAddr & ~3u, data);
        }
        else{
            if(currentSourceAddr >= 0x2000000){
                lastRead = cbus->readHalfWordFromDMA(currentSourceAddr);
                lastRead |= lastRead << 16; // Halbwort Wird in Latch dupliziert
            }
            Word data = lastRead;
            if(currentSourceAddr < 0x2000000 && currentDestAddr & 2)
                data >>= 16;
            cbus->writeHalfWordFromDMA(currentDestAddr & ~1u, data);
        }
    
        // if(startTiming != DMA_SOUND_FIFO)
        currentDestAddr += destIncrement;

        currentSourceAddr += sourceIncrement;

        currentSourceAddr &= isDMA7() ? srcMasks[dmaIndex] : 0xFFFFFFE;
        currentDestAddr &= isDMA7() ? destMasks[dmaIndex] : 0xFFFFFFE;
        
        currentCount++;
    }

    // Ende
    if(doesRepeat()){
        bool reloadDAD = getDestAddrControl() == DEST_INCREMENT_RELOAD;
        resetInternalCounters(false, reloadDAD);
        if(startTiming == DMA_IMMEDIATE){
            bus->scheduler->scheduleEvent({.timePoint = remaining + 3, .type = schedulerEvent, .args={.index = this->dmaIndex}});
        }
    }
    else{
        Control.raw &= ~(1u << 15);
    }
    
    if(irqOnEnd()){
        cbus->setIF(8 + dmaIndex, true);
    }

    cbus->addCPUCycles(remaining);
}

template<typename ConcreteBus>
void DMAChannel<ConcreteBus>::printStartTiming(){
    auto s = getStartTiming();
    switch(s){
        case DMA_HBLANK_OR_CARTRIDGE_SLOT:
            std::cout << "Hblank or Cartridge\n";
            break;
        case DMA_SYNC_OR_WIRELESS_SPECIAL:
            std::cout << "Sync or Wireless\n";
            break;
        case DMA_IMMEDIATE:
            std::cout << "Immediate\n";
            break;
        case DMA_MAIN_MEMORY_DISPLAY:
            std::cout << "Main Memory Display\n";
            break;
        case DMA_VBLANK:
            std::cout << "VBLANK\n";
            break;
        case DMA_DS_CARTRIDGE_SLOT:
            std::cout << "Ds Cartridge Slot\n";
            break;
        case DMA_GBA_CARTRIDGE_SLOT:
            std::cout << "Gba Cartridge Slot\n";
            break;
        case DMA_GEOMETRY_COMMAND_FIFO:
            std::cout << "Geometry Command Fifo\n";
            break;
    }
}

namespace nds{
    template class DMAChannel<Bus7>;
    template class DMAChannel<Bus9>;
}

