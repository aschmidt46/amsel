#include "testbus.h"

using namespace gba;

void gba::TestBus::setTransactions(std::vector<Transaction> transactions)
{
    this->transactions = transactions;
}

gba::TestBus::TestBus(std::vector<Transaction> transactions)
{
    setTransactions(transactions);
}

void gba::TestBus::writeByte(Word addr, Byte val)
{
    auto found = std::find_if(transactions.begin(), transactions.end(), [&](const Transaction &t) -> bool {
        return t.size == 1 && t.kind == 2 && t.addr == addr && ((Byte)t.data) == val;
    });
    if(found == transactions.end()){
        hadError = true;
    }
}

Byte gba::TestBus::readByte(Word addr)
{
    auto found = std::find_if(transactions.begin(), transactions.end(), [&](const Transaction &t) -> bool {
        return t.size == 1 && t.kind <= 1 && t.addr == addr;
    });
    if(found == transactions.end()){
        hadError = true;
        return 0;
    }
    return found->data;
}

void gba::TestBus::writeHalfWord(Word addr, HalfWord val)
{
    auto found = std::find_if(transactions.begin(), transactions.end(), [&](const Transaction &t) -> bool {
        return t.size == 2 && t.kind == 2 && t.addr == addr && ((HalfWord)t.data) == val;
    });
    if(found == transactions.end()){
        hadError = true;
    }
}

HalfWord gba::TestBus::readHalfWord(Word addr)
{
    auto found = std::find_if(transactions.begin(), transactions.end(), [&](const Transaction &t) -> bool {
        return t.size == 2 && t.kind <= 1 && t.addr == addr;
    });
    if(found == transactions.end()){
        hadError = true;
        return 0;
    }
    return found->data;
}

void gba::TestBus::writeWord(Word addr, Word val)
{
    auto found = std::find_if(transactions.begin(), transactions.end(), [&](const Transaction &t) -> bool {
        return t.size == 4 && t.kind == 2 && t.addr == addr && ((Word)t.data) == val;
    });
    if(found == transactions.end()){
        hadError = true;
    }
}

Word gba::TestBus::readWord(Word addr)
{
    auto found = std::find_if(transactions.begin(), transactions.end(), [&](const Transaction &t) -> bool {
        return t.size == 4 && t.kind <= 1 && t.addr == addr;
    });
    if(found == transactions.end()){
        hadError = true;
        return 0;
    }
    return found->data;
}
