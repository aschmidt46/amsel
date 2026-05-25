#pragma once
#include <cstdint>
#include <vector>
#include "../arm/arm7tdmi.h"
#include "nlohmann/json.hpp"

using json = nlohmann::json;


struct CpuTransition{
    public:
    gba::CpuRegisterState initial;
    gba::CpuRegisterState final;
    std::vector<gba::Transaction> transactions;
    uint32_t opcode;
    uint32_t base_addr;

    gba::CpuRegisterState stateFromJson(const json &data){
        gba::CpuRegisterState state;
        int i = 0;
        for(auto r : data["R"]){
            state.R[i] = r.get<uint32_t>();
            i++;
        }
        i = 0;
        for(auto r : data["R_fiq"]){
            state.R_fiq[i] = r.get<uint32_t>();
            i++;
        }
        i = 0;
        for(auto r : data["R_svc"]){
            state.R_svc[i] = r.get<uint32_t>();
            i++;
        }
        i = 0;
        for(auto r : data["R_abt"]){
            state.R_abt[i] = r.get<uint32_t>();
            i++;
        }
        i = 0;
        for(auto r : data["R_irq"]){
            state.R_irq[i] = r.get<uint32_t>();
            i++;
        }
        i = 0;
        for(auto r : data["R_und"]){
            state.R_und[i] = r.get<uint32_t>();
            i++;
        }
        state.CPSR = data["CPSR"].get<uint32_t>();
        i = 0;
        for(auto r : data["SPSR"]){
            state.SPSR[i] = r.get<uint32_t>();
            i++;
        }
        i = 0;
        for(auto r : data["pipeline"]){
            state.Pipeline[i] = r.get<uint32_t>();
            i++;
        }
        return state;
    }

    gba::Transaction transactionFromJson(const json &data){
        gba::Transaction t;
        t.kind = data["kind"].get<int>();
        t.size = data["size"].get<int>();
        t.addr = data["addr"].get<uint32_t>();
        t.data = data["data"].get<uint32_t>();
        t.cycle = data["cycle"].get<size_t>();
        t.access = data["access"].get<uint32_t>();
        return t;
    }

    CpuTransition(const json &data){
        initial = stateFromJson(data["initial"]);
        final = stateFromJson(data["final"]);
        transactions = std::vector<gba::Transaction>(0);
        for(auto t : data["transactions"]){
            transactions.push_back(transactionFromJson(t));
        }
        opcode = data["opcode"].get<uint32_t>();
        base_addr= data["base_addr"].get<uint32_t>();
    };
};

static std::vector<CpuTransition> parseSingleStepTest(const json &data){
    std::vector<CpuTransition> v;
    for(auto e : data){
        v.push_back(CpuTransition(e));
    }
    return v;
}
