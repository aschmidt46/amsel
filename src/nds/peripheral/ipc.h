#pragma once
#include "../bus_types.h"
#include <queue>

namespace nds{
    class SharedBus;
    class IPC{
        Byte nds7Out = 0;
        Byte nds9Out = 0;
        bool enableIRQ7to9 = false;
        bool enableIRQ9to7 = false;
        SharedBus* bus;

        HalfWord IPCFIFOCNT_7 = 0;
        HalfWord IPCFIFOCNT_9 = 0;

        std::queue<Word> sendFifo7;
        Word sendFifo7recent = 0;
        std::queue<Word> sendFifo9;
        Word sendFifo9recent = 0;

        Byte readIPCSYNCLower(bool isNDS9);
        Byte readIPCSYNCHigher(bool isNDS9);
        void writeIPCSYNCLower(bool isNDS9, Byte val);
        void writeIPCSYNCHigher(bool isNDS9, Byte val);
        Byte readIPCFIFOCNTLower(bool isNDS9);
        Byte readIPCFIFOCNTHigher(bool isNDS9);
        void writeIPCFIFOCNTLower(bool isNDS9, Byte val);
        void writeIPCFIFOCNTHigher(bool isNDS9, Byte val);

        public:
        IPC(SharedBus* bus) : bus(bus){};

        Byte readReg(Word addr, bool isNDS9);
        void writeReg(Word addr, Byte val, bool isNDS9);


        Word readFIFORECV(bool isNDS9);
        void writeFIFOSEND(bool isNDS9, Word val);
    };
}