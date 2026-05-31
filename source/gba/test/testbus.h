#pragma once
#include "../ibus.h"
#include "gba/arm/arm7tdmi.h"

namespace gba{
    class TestBus : public IBus{
        private:
        std::vector<Transaction> transactions;
        
        void logErrorNotTransacted(int index);
        void logErrorUnknownTransaction(Word size, Word addr, Word val, std::string kind);
        void logErrorDuplicateTransaction(int index);

      public:
        std::vector<size_t> completed;
        std::string logs = "";
        bool hadError = false;
        void setTransactions(std::vector<Transaction> transactions);
        TestBus(std::vector<Transaction> transactions);
        TestBus() = default;
        void writeByte(Word addr, Byte val) override;
        Byte readByte(Word addr) override;
        void writeHalfWord(Word addr, HalfWord val) override;
        HalfWord readHalfWord(Word addr) override;
        void writeWord(Word addr, Word val) override;
        Word readWord(Word addr) override;

        void finalize();
    };
}
