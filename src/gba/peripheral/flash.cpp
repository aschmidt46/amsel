#include "flash.h"
#include <iostream>
#include "framework/stringlib.h"

using namespace gba;

void gba::Flash::parseCommand(Word addr, Byte val)
{
    switch(val){
        case 0x90:{
            chipIdent = true;
            state = IDLE;
            return;
        }
        case 0xF0:{
            chipIdent = false;
            state = IDLE;
            return;
        }
        case 0x80:{
            state = RECEIVE_ERASE;
            return;
        }
        case 0xA0:{
            state = AWAIT_WRITE;
            return;
        }
        case 0xB0:{
            state = AWAIT_BANK_SWITCH;
            return;
        }
        default:{
            state = IDLE;
            return;
        }
    }
}

inline void gba::Flash::OnWrite5555(Byte val)
{
    switch(state){
        case IDLE:
            if(val == 0xAA){
                state = PREPARE_1;
            }
            return;
        case RECEIVE_ERASE:
            if(val == 0xAA){
                state = ERASE_PREPARE_1;
            }
            return;
        case PREPARE_2:
            parseCommand(0xE005555, val);
            return;
        case ERASE_PREPARE_2:
            if(val == 0x10)
                memory = std::vector<Byte>(SIZE, 0xFF);
            state = IDLE;
            return;
        default:
            state = IDLE;
            return;
    }
}

void gba::Flash::OnWrite2AAA(Byte val)
{
    switch(state){
        case PREPARE_1:
            if(val == 0x55){
                state = PREPARE_2;
            }
            return;
        case ERASE_PREPARE_1:
            if(val == 0x55){
                state = ERASE_PREPARE_2;
            }
            return;
        default:
            state = IDLE;
            return;
    }
}

void gba::Flash::OnWriteN000(Word addr, Byte val)
{
    if(val == 0x30 && state == ERASE_PREPARE_2){
        Word start = (addr - 0x0E000000);
        for(size_t i = start; i < start + 0x1000; i++){
            memory[i + bank * 0x10000] = 0xFF;
        }
        state = IDLE;
        return;
    }
}

Byte gba::Flash::OnRead(Word addr)
{
    if(addr == 0x0E000000 && chipIdent){
        if(SIZE == 0x10000) return 0x32; // 64KB
        else return 0x62;
    }
    if(addr == 0x0E000001 && chipIdent){
        if(SIZE == 0x10000) return 0x1B; // 64KB
        else return 0x13;
    }

    return memory[addr - 0x0E000000 + bank * 0x10000];
}

void gba::Flash::OnWrite(Word addr, Byte val)
{
    if(state == AWAIT_WRITE){
        memory[addr - 0x0E000000 + bank * 0x10000] = val;
        state = IDLE;
    }
    else if(state == AWAIT_BANK_SWITCH && SIZE > 0x10000 && addr == 0x0E000000){
        bank = val & 1u;
        state = IDLE;
    }
    else if(addr == 0x0E002AAA){
        OnWrite2AAA(val);
    }
    else if(addr == 0x0E005555){
        OnWrite5555(val);
    }
    else if(addr % 0x1000 == 0){
        OnWriteN000(addr, val);
    }
}

void gba::Flash::loadSave(const std::vector<uint8_t> &saveData)
{
    memory = saveData;
}

std::vector<Byte> gba::Flash::getSaveData()
{
    return memory;
}

