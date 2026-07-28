#include "general_purpose.h"

using namespace gba;

Byte GeneralPurpose32::OnReadByte(Word addr){
    if(addr == start){
        return raw;
    }
    else if(addr == start + 1){
        return raw >> 8;
    }
    else if(addr == start + 2){
        return raw >> 16;
    }
    else if(addr == start + 3){
        return raw >> 24;
    }
    else return 0;
}

void GeneralPurpose32::OnWriteByte(Word addr, Byte val){
    if(addr == start){
        raw = (raw & ~0xFFu) | val;
    }
    else if(addr == start + 1){
        raw = (raw & ~0xFF00u) | (Word(val) << 8);
    }
    else if(addr == start + 2){
        raw = (raw & ~0xFF0000u) | (Word(val) << 16);
    }
    else if(addr == start + 3){
        raw = (raw & ~0xFF000000u) | (Word(val) << 24);
    }
}

Byte GeneralPurpose16::OnReadByte(Word addr){
    if(addr == start){
        return raw;
    }
    else if(addr == start + 1){
        return raw >> 8;
    }
    else return 0;
}

void GeneralPurpose16::OnWriteByte(Word addr, Byte val){
    if(addr == start){
        raw = (raw & ~0xFFu) | val;
    }
    else if(addr == start + 1){
        raw = (raw & ~0xFF00u) | (HalfWord(val) << 8);
    }
}