#include "testbus.h"
#include "logging.h"
#include <algorithm>

using namespace gba;

void gba::TestBus::setTransactions(std::vector<Transaction> transactions)
{
    this->completed.clear();
    this->logs = "";
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
        logErrorUnknownTransaction(1, addr, val, "Write");
        return;
    }
    size_t index = std::distance(transactions.begin(), found);
    if(std::find(completed.begin(), completed.end(), index) != completed.end()){
        logErrorDuplicateTransaction(index);
    }
    else completed.push_back(index);
}

Byte gba::TestBus::readByte(Word addr)
{
    auto found = std::find_if(transactions.begin(), transactions.end(), [&](const Transaction &t) -> bool {
        return t.size == 1 && t.kind <= 1 && t.addr == addr;
    });
    if(found == transactions.end()){
        logErrorUnknownTransaction(1, addr, 0, "Read");
        return 0;
    }
    size_t index = std::distance(transactions.begin(), found);
    if(std::find(completed.begin(), completed.end(), index) != completed.end()){
        logErrorDuplicateTransaction(index);
    }
    else completed.push_back(index);
    return found->data;
}

void gba::TestBus::writeHalfWord(Word addr, HalfWord val)
{
    auto found = std::find_if(transactions.begin(), transactions.end(), [&](const Transaction &t) -> bool {
        return t.size == 2 && t.kind == 2 && t.addr == addr && ((HalfWord)t.data) == val;
    });
    if(found == transactions.end()){
        logErrorUnknownTransaction(2, addr, val, "Write");
        return;
    }
    size_t index = std::distance(transactions.begin(), found);
    if(std::find(completed.begin(), completed.end(), index) != completed.end()){
        logErrorDuplicateTransaction(index);
    }
    else completed.push_back(index);
}

HalfWord gba::TestBus::readHalfWord(Word addr)
{
    auto found = std::find_if(transactions.begin(), transactions.end(), [&](const Transaction &t) -> bool {
        return t.size == 2 && t.kind <= 1 && t.addr == addr;
    });
    if(found == transactions.end()){
        logErrorUnknownTransaction(2, addr, 0, "Read");
        return 0;
    }
    size_t index = std::distance(transactions.begin(), found);
    if(std::find(completed.begin(), completed.end(), index) != completed.end()){
        logErrorDuplicateTransaction(index);
    }
    else completed.push_back(index);
    return found->data;
}

void gba::TestBus::writeWord(Word addr, Word val)
{
    auto found = std::find_if(transactions.begin(), transactions.end(), [&](const Transaction &t) -> bool {
        return t.size == 4 && t.kind == 2 && t.addr == addr && ((Word)t.data) == val;
    });
    if(found == transactions.end()){
        logErrorUnknownTransaction(4, addr, val, "Write");
        return;
    }
    size_t index = std::distance(transactions.begin(), found);
    if(std::find(completed.begin(), completed.end(), index) != completed.end()){
        logErrorDuplicateTransaction(index);
    }
    else completed.push_back(index);
}

Word gba::TestBus::readWord(Word addr)
{
    auto found = std::find_if(transactions.begin(), transactions.end(), [&](const Transaction &t) -> bool {
        return t.size == 4 && t.kind <= 1 && t.addr == addr;
    });
    if(found == transactions.end()){
        logErrorUnknownTransaction(4, addr, 0, "Read");
        return 0;
    }
    size_t index = std::distance(transactions.begin(), found);
    if(std::find(completed.begin(), completed.end(), index) != completed.end()){
        logErrorDuplicateTransaction(index);
    }
    else completed.push_back(index);
    return found->data;
}

std::string printKind(int kind){
    switch(kind){
        case 0:
            return "Instruction Read";
        case 1:
            return "General Read";
        case 2:
            return "Write";
        default:
            return "?";
    }
}

void gba::TestBus::logErrorNotTransacted(int index)
{
    hadError = true;
    std::string str = "";
    str += "Transaktion nicht durchgeführt:\n";
    str += "\tKind:\t"+printKind(transactions[index].kind)+"\n";
    str += "Size:\t"+std::to_string(transactions[index].size)+"\n";
    str += "Addr:\t"+getHex(transactions[index].addr, 8)+"\n";
    str += "Data:\t"+getHex(transactions[index].data, 8)+"\n\n";
    logs += str;
}

void gba::TestBus::logErrorUnknownTransaction(Word size, Word addr, Word val, std::string kind)
{
    hadError = true;
    std::string str = "";
    str += "Unbekannte Transaktion:\n";
    str += "\tKind:\t"+kind+"\n";
    str += "Size:\t"+std::to_string(size)+"\n";
    str += "Addr:\t"+getHex(addr, 8)+"\n";
    str += "Data:\t"+getHex(val, 8)+"\n\n";
    logs += str;
}

void gba::TestBus::finalize()
{
    for(auto it = transactions.begin(); it != transactions.end(); it++){
        size_t index = std::distance(transactions.begin(), it);
        if(std::find(completed.begin(), completed.end(), index) == completed.end()){
            if(it->kind != 0){ // Instruction Reads sind mir aktuell egal
                logErrorNotTransacted((int) index);
            }
        }
    }
};
void gba::TestBus::logErrorDuplicateTransaction(int index)
{
    // if(transactions[index].kind != 0){
        hadError = true;
        std::string str = "";
        str += "Transaktion mehrfach durchgeführt:\n";
        str += "\tKind:\t"+printKind(transactions[index].kind)+"\n";
        str += "Size:\t"+std::to_string(transactions[index].size)+"\n";
        str += "Addr:\t"+getHex(transactions[index].addr, 8)+"\n";
        str += "Data:\t"+getHex(transactions[index].data, 8)+"\n\n";
        logs += str;
    // }
};