#pragma once
#include "../ibus.h"

namespace gba{
    class TestBus : public IBus{
        private:
        std::vector<Transaction> transactions;
        public:
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
    };
}
