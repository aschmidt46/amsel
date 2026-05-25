#include "logging.h"
#include <format>
#include <algorithm>

using namespace gba;

void separateEveryN(std::string& str, int interval, const char* sep)
{
    for (int i = 0; i < str.size(); i += interval)
        str.insert(i, sep);
}

std::string thex(uintptr_t input)
{
    std::string str = std::format("{:x}", input);
    std::transform(str.begin(), str.end(), str.begin(), ::toupper);
    return str;
}

std::string thexNorm(std::string s, int n)
{
    while (s.size() < n)
        s = "0" + s;
    return s;
}

std::string getHex(size_t input, int length){
    return "0x"+thexNorm(thex(input), length);
}

std::string printState(gba::CpuRegisterState state){
    std::string res;
    for(int i = 0; i < 16; i++){
        res += "R"+std::to_string(i)+":\t\t"+getHex(state.R[i],8)+"\n";
    }
    res +="\n";
    for(int i = 0; i < 7; i++){
        res += "R"+std::to_string(i+8)+"_fiq: \t"+getHex(state.R_fiq[i],8)+"\n";
    }
    for(int i = 0; i < 2; i++){
        res += "R"+std::to_string(i+13)+"_svc:\t"+getHex(state.R_svc[i],8)+"\n";
    }
    for(int i = 0; i < 2; i++){
        res += "R"+std::to_string(i+13)+"_abt:\t"+getHex(state.R_abt[i],8)+"\n";
    }
    for(int i = 0; i < 2; i++){
        res += "R"+std::to_string(i+13)+"_irq:\t"+getHex(state.R_irq[i],8)+"\n";
    }
    for(int i = 0; i < 2; i++){
        res += "R"+std::to_string(i+13)+"_und:\t"+getHex(state.R_und[i],8)+"\n";
    }
    res += "\n";
    res += "SPSR_fiq:\t"+getHex(state.SPSR[0],8)+"\n";
    res += "SPSR_svc:\t"+getHex(state.SPSR[1],8)+"\n";
    res += "SPSR_abt:\t"+getHex(state.SPSR[2],8)+"\n";
    res += "SPSR_irq:\t"+getHex(state.SPSR[3],8)+"\n";
    res += "SPSR_und:\t"+getHex(state.SPSR[4],8)+"\n";

    res += "Pipeline[0]:\t"+getHex(state.Pipeline[0],8)+"\n";
    res += "Pipeline[1]:\t"+getHex(state.Pipeline[1],8)+"\n";
    return res;
}


std::string printDiff(CpuRegisterState was, CpuRegisterState is, CpuRegisterState test){
    std::string res;
    res += "----------------------------------------------------\n\n";

    for(int i = 0; i < 16; i++){
        if(is.R[i] != test.R[i]){
            res += "Nutzerregister R"+std::to_string(i)+" weicht ab.\n";
            res += "\tWar:\t"+getHex(was.R[i],8)+"\n";
            res += "\tSoll:\t"+getHex(test.R[i],8)+"\n";
            res += "\tIst:\t"+getHex(is.R[i],8)+"\n\n";
        }
    }

    for(int i = 0; i < 7; i++){
        if(is.R_fiq[i] != test.R_fiq[i]){
            res += "Register R"+std::to_string(i+8)+"_fiq weicht ab.\n";
            res += "\tWar:\t"+getHex(was.R_fiq[i],8)+"\n";
            res += "\tSoll:\t"+getHex(test.R_fiq[i],8)+"\n";
            res += "\tIst:\t"+getHex(is.R_fiq[i],8)+"\n\n";
        }
    }

    for(int i = 0; i < 2; i++){
        if(is.R_svc[i] != test.R_svc[i]){
            res += "Register R"+std::to_string(i+13)+"_svc weicht ab.\n";
            res += "\tWar:\t"+getHex(was.R_svc[i],8)+"\n";
            res += "\tSoll:\t"+getHex(test.R_svc[i],8)+"\n";
            res += "\tIst:\t"+getHex(is.R_svc[i],8)+"\n\n";
        }
    }

    for(int i = 0; i < 2; i++){
        if(is.R_abt[i] != test.R_abt[i]){
            res += "Register R"+std::to_string(i+13)+"_abt weicht ab.\n";
            res += "\tWar:\t"+getHex(was.R_abt[i],8)+"\n";
            res += "\tSoll:\t"+getHex(test.R_abt[i],8)+"\n";
            res += "\tIst:\t"+getHex(is.R_abt[i],8)+"\n\n";
        }
    }

    for(int i = 0; i < 2; i++){
        if(is.R_irq[i] != test.R_irq[i]){
            res += "Register R"+std::to_string(i+13)+"_irq weicht ab.\n";
            res += "\tWar:\t"+getHex(was.R_irq[i],8)+"\n";
            res += "\tSoll:\t"+getHex(test.R_irq[i],8)+"\n";
            res += "\tIst:\t"+getHex(is.R_irq[i],8)+"\n\n";
        }
    }

    for(int i = 0; i < 2; i++){
        if(is.R_und[i] != test.R_und[i]){
            res += "Register R"+std::to_string(i+13)+"_und weicht ab.\n";
            res += "\tWar:\t"+getHex(was.R_und[i],8)+"\n";
            res += "\tSoll:\t"+getHex(test.R_und[i],8)+"\n";
            res += "\tIst:\t"+getHex(is.R_und[i],8)+"\n\n";
        }
    }

    if(is.CPSR != test.CPSR){
        res += "CPSR weicht ab.\n";
        res += "\tWar:\t"+getHex(was.CPSR,8)+"\n";
        res += "\tSoll:\t"+getHex(test.CPSR,8)+"\n";
        res += "\tIst:\t"+getHex(is.CPSR,8)+"\n\n";
    }

    if(is.SPSR[0] != test.SPSR[0]){
        res += "SPSR_fiq weicht ab.\n";
        res += "\tWar:\t"+getHex(was.SPSR[0],8)+"\n";
        res += "\tSoll:\t"+getHex(test.SPSR[0],8)+"\n";
        res += "\tIst:\t"+getHex(is.SPSR[0],8)+"\n\n";
    }

    if(is.SPSR[1] != test.SPSR[1]){
        res += "SPSR_svc weicht ab.\n";
        res += "\tWar:\t"+getHex(was.SPSR[1],8)+"\n";
        res += "\tSoll:\t"+getHex(test.SPSR[1],8)+"\n";
        res += "\tIst:\t"+getHex(is.SPSR[1],8)+"\n\n";
    }

    if(is.SPSR[2] != test.SPSR[2]){
        res += "SPSR_abt weicht ab.\n";
        res += "\tWar:\t"+getHex(was.SPSR[2],8)+"\n";
        res += "\tSoll:\t"+getHex(test.SPSR[2],8)+"\n";
        res += "\tIst:\t"+getHex(is.SPSR[2],8)+"\n\n";
    }
    
    if(is.SPSR[3] != test.SPSR[3]){
        res += "SPSR_irq weicht ab.\n";
        res += "\tWar:\t"+getHex(was.SPSR[3],8)+"\n";
        res += "\tSoll:\t"+getHex(test.SPSR[3],8)+"\n";
        res += "\tIst:\t"+getHex(is.SPSR[3],8)+"\n\n";
    }

    if(is.SPSR[4] != test.SPSR[4]){
        res += "SPSR_und weicht ab.\n";
        res += "\tWar:\t"+getHex(was.SPSR[4],8)+"\n";
        res += "\tSoll:\t"+getHex(test.SPSR[4],8)+"\n";
        res += "\tIst:\t"+getHex(is.SPSR[4],8)+"\n\n";
    }

    if(is.Pipeline[0] != test.Pipeline[0]){
        res += "Pipeline[0] weicht ab.\n";
        res += "\tWar:\t"+getHex(was.Pipeline[0],8)+"\n";
        res += "\tSoll:\t"+getHex(test.Pipeline[0],8)+"\n";
        res += "\tIst:\t"+getHex(is.Pipeline[0],8)+"\n\n";
    }

    if(is.Pipeline[1] != test.Pipeline[1]){
        res += "Pipeline[1] weicht ab.\n";
        res += "\tWar:\t"+getHex(was.Pipeline[1],8)+"\n";
        res += "\tSoll:\t"+getHex(test.Pipeline[1],8)+"\n";
        res += "\tIst:\t"+getHex(is.Pipeline[1],8)+"\n\n";
    }

    res += "----------------------------------------------------\n";

    res += "Zustand Vorher:\n";
    res += printState(was)+"\n\n";
    res += "Zustand Nachher:\n";
    res += printState(is);

    res += "----------------------------------------------------\n";
    return res;
}

std::string printTransactions(const std::vector<gba::Transaction> &transactions)
{
    std::string res = "";
    int count = 0;
    for(const auto &t : transactions){
        count++;
        res += "Transaktion "+std::to_string(count)+":\n";
        std::string kind;
        if(t.kind == 0) kind = "Instruction Read";
        else if(t.kind == 1) kind = "General Read";
        else kind = "Write";
        res += "Kind:\t"+kind+"\n";
        res += "Size:\t"+std::to_string(t.size)+"\n";
        res += "Addr:\t"+getHex(t.addr, 8)+"\n";
        res += "Data:\t"+getHex(t.data, 8)+"\n\n";
    }
    res += "----------------------------------------------------\n";
    return res;
}
