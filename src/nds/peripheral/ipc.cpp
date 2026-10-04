#include "ipc.h"
#include "../bus.h"

using namespace nds;

Byte nds::IPC::readIPCSYNCLower(bool isNDS9)
{
    if(isNDS9){
        return (nds7Out & 0b111);   
    }
    else{
        return (nds9Out & 0b111);
    }
}

Byte nds::IPC::readIPCSYNCHigher(bool isNDS9)
{
    if(isNDS9){
        return (nds9Out & 0b111) | (Byte(enableIRQ7to9) << 6);
    }
    else{
        return (nds7Out & 0b111) | (Byte(enableIRQ9to7) << 6);
    }
}

void nds::IPC::writeIPCSYNCLower(bool isNDS9, Byte val)
{
}

void nds::IPC::writeIPCSYNCHigher(bool isNDS9, Byte val)
{
    if(isNDS9){
        nds9Out = val & 0b111;
        enableIRQ7to9 = ((val >> 6) & 1) > 0;
        if(((val >> 5) & 1) > 0 && enableIRQ9to7){
            bus->setIFSingleBus(16, true, false);
        }
    }
    else{
        nds7Out = val & 0b111;
        enableIRQ9to7 = ((val >> 6) & 1) > 0;
        if(((val >> 5) & 1) > 0 && enableIRQ7to9){
            bus->setIFSingleBus(16, true, true);
        }
    }
}

Byte nds::IPC::readIPCFIFOCNTLower(bool isNDS9)
{
    if(isNDS9){
        return (sendFifo9.empty()) | ((sendFifo9.size() >= 16) << 1) | (IPCFIFOCNT_9 & 4);
    }
    else{
        return (sendFifo7.empty()) | ((sendFifo7.size() >= 16) << 1) | (IPCFIFOCNT_7 & 4);
    }
}

Byte nds::IPC::readIPCFIFOCNTHigher(bool isNDS9)
{
    if(isNDS9){
        return (sendFifo7.empty()) | ((sendFifo7.size() >= 16) << 1) | ((IPCFIFOCNT_9 >> 8) & 4) | ((IPCFIFOCNT_9 >> 8) & 64) | ((IPCFIFOCNT_9 >> 8) & 128);
    }
    else{
        return (sendFifo9.empty()) | ((sendFifo9.size() >= 16) << 1) | ((IPCFIFOCNT_7 >> 8) & 4) | ((IPCFIFOCNT_7 >> 8) & 64) | ((IPCFIFOCNT_7 >> 8) & 128);
    }
}

void nds::IPC::writeIPCFIFOCNTLower(bool isNDS9, Byte val)
{
    if(isNDS9){
        bool before = (IPCFIFOCNT_9 & 4) && sendFifo9.empty();
        IPCFIFOCNT_9 = (IPCFIFOCNT_9 & ~4u) | (val & 4); // set irq enable
        if((val & 8) > 0){
            sendFifo9 = {};
            sendFifo9recent = 0;
        }
        bool after = (IPCFIFOCNT_9 & 4) && sendFifo9.empty();
        if(!before && after){ // rising edge
            bus->setIFSingleBus(17, true, true);
        }
    }
    else{
        bool before = (IPCFIFOCNT_7 & 4) && sendFifo7.empty();
        IPCFIFOCNT_7 = (IPCFIFOCNT_7 & ~4u) | (val & 4); // set irq enable
        if((val & 8) > 0){
            sendFifo7 = {};
            sendFifo7recent = 0;
        }
        bool after = (IPCFIFOCNT_7 & 4) && sendFifo7.empty();
        if(!before && after){ // rising edge
            bus->setIFSingleBus(17, true, false);
        }
    }
}

void nds::IPC::writeIPCFIFOCNTHigher(bool isNDS9, Byte val)
{
    if(isNDS9){
        bool before = ((IPCFIFOCNT_9 >> 8) & 4) && !sendFifo7.empty();
        IPCFIFOCNT_9 = (IPCFIFOCNT_9 & ~(0b11000100 << 8)) | (HalfWord(val & 0b11000100) << 8); // set irq enable
        bool after = ((IPCFIFOCNT_9 >> 8) & 4) && !sendFifo7.empty();
        if(!before && after){ // rising edge
            bus->setIFSingleBus(18, true, true);
        }
    }
    else{
        bool before = ((IPCFIFOCNT_7 >> 8) & 4) && !sendFifo9.empty();
        IPCFIFOCNT_7 = (IPCFIFOCNT_7 & ~(0b11000100 << 8)) | (HalfWord(val & 0b11000100) << 8); // set irq enable
        bool after = ((IPCFIFOCNT_7 >> 8) & 4) && !sendFifo9.empty();
        if(!before && after){ // rising edge
            bus->setIFSingleBus(18, true, false);
        }
    }
}

Byte nds::IPC::readReg(Word addr, bool isNDS9)
{
    if(addr == 0x4000180){
        return readIPCSYNCLower(isNDS9);
    }
    else if(addr == 0x4000181){
        return readIPCSYNCHigher(isNDS9);
    }
    else if(addr == 0x4000184){
        return readIPCFIFOCNTLower(isNDS9);
    }
    else if(addr == 0x4000185){
        return readIPCFIFOCNTHigher(isNDS9);
    }
    return 0;
}

void nds::IPC::writeReg(Word addr, Byte val, bool isNDS9)
{
    if(addr == 0x4000180){
        writeIPCSYNCLower(isNDS9, val);
    }
    else if(addr == 0x4000181){
        writeIPCSYNCHigher(isNDS9, val);
    }
    else if(addr == 0x4000184){
        writeIPCFIFOCNTLower(isNDS9, val);
    }
    else if(addr == 0x4000185){
        writeIPCFIFOCNTHigher(isNDS9, val);
    }
}

Word nds::IPC::readFIFORECV(bool isNDS9)
{
    if(isNDS9){
        if(!sendFifo7.empty()){
            Word elem = sendFifo7.front();
            sendFifo7recent = elem;
            if(IPCFIFOCNT_9 >> 15)
                sendFifo7.pop();
            if((IPCFIFOCNT_7 & 4) && sendFifo7.empty()){
                bus->setIFSingleBus(17, true, false); // fifo 7 empty -> nds7 irq
            }
            return elem;
        }
        else{
            IPCFIFOCNT_9 |= 1u << 14;
            return sendFifo7recent;
        }
    }
    else{
        if(!sendFifo9.empty()){
            Word elem = sendFifo9.front();
            sendFifo9recent = elem;
            if(IPCFIFOCNT_7 >> 15)
                sendFifo9.pop();
            if((IPCFIFOCNT_9 & 4) && sendFifo9.empty()){
                bus->setIFSingleBus(17, true, true); // fifo 9 empty -> nds9 irq
            }
            return elem;
        }
        else{
            IPCFIFOCNT_7 |= 1u << 14;
            return sendFifo9recent;
        }
    }
}

void nds::IPC::writeFIFOSEND(bool isNDS9, Word val)
{
    if(isNDS9){
        if(IPCFIFOCNT_9 >> 15){
            if(sendFifo9.size() < 16){
                sendFifo9.push(val);
                if(sendFifo9.size() == 1 && ((IPCFIFOCNT_7 >> 10) & 1))
                    bus->setIFSingleBus(18, true, false); // fifo 7 not empty -> nds7 irq
            }
            else{
                IPCFIFOCNT_9 |= 1u << 14;
            }
        }
    }
    else{
        if(IPCFIFOCNT_7 >> 15){
            if(sendFifo7.size() < 16){
                sendFifo7.push(val);
                if(sendFifo7.size() == 1 && ((IPCFIFOCNT_9 >> 10) & 1))
                    bus->setIFSingleBus(18, true, true); // fifo 9 not empty -> nds9 irq
            }
            else{
                IPCFIFOCNT_7 |= 1u << 14;
            }
        }
    }
}
