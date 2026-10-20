#include "cartridge.h"
#include "../bus.h"
#include "../bus7.h"
#include <cstring>

using namespace nds;

void nds::Cartridge::encrypt64(Byte* ptr)
{
    Word Y = *(Word*)(ptr+0);
    Word X = *(Word*)(ptr+4);
    for(size_t I = 0; I <= 0xF; I++){
        Word Z=*((Word*)(keybuf.data()+I*4)) ^ X;
        X=(*(Word*)(keybuf.data()+0x048+((Z >> 24) & 0xFF)*4));
        X=(*(Word*)(keybuf.data()+0x448+((Z >> 16) & 0xFF)*4)) + X;
        X=(*(Word*)(keybuf.data()+0x848+((Z >>  8) & 0xFF)*4)) ^ X;
        X=(*(Word*)(keybuf.data()+0xC48+((Z >>  0) & 0xFF)*4)) + X;
        X=Y ^ X;
        Y=Z;
    }
    *(Word*)(ptr+0) = X ^ (*(Word*)(keybuf.data()+0x40));
    *(Word*)(ptr+4) = Y ^ (*(Word*)(keybuf.data()+0x44));
}

void nds::Cartridge::decrypt64(Byte* ptr)
{
    Word Y = *(Word*)(ptr+0);
    Word X = *(Word*)(ptr+4);
    for(size_t I = 0x11; I >= 0x02; I--){
        Word Z=*((Word*)(keybuf.data()+I*4)) ^ X;
        X=(*(Word*)(keybuf.data()+0x048+((Z >> 24) & 0xFF)*4));
        X=(*(Word*)(keybuf.data()+0x448+((Z >> 16) & 0xFF)*4)) + X;
        X=(*(Word*)(keybuf.data()+0x848+((Z >>  8) & 0xFF)*4)) ^ X;
        X=(*(Word*)(keybuf.data()+0xC48+((Z >>  0) & 0xFF)*4)) + X;
        X=Y ^ X;
        Y=Z;
    }
    *(Word*)(ptr+0) = X ^ (*(Word*)(keybuf.data()+0x4));
    *(Word*)(ptr+4) = Y ^ (*(Word*)(keybuf.data()+0x0));
}

void nds::Cartridge::applyKeycode(Word modulo)
{
    encrypt64(keycode.data()+4);
    encrypt64(keycode.data()+0);
    std::array<Byte, 8> scratch = {{0,0,0,0,0,0,0,0}};                          //;S=0 (64bit)
    for(size_t I = 0; I <= 0x44; I += 4){
        *(Word*)(keybuf.data()+I)= *(Word*)(keybuf.data()+I) ^ std::byteswap(*(Word*)(keycode.data()+(I % modulo)));                          //;xor with reversed byte-order (bswap)
    }
    for(size_t I = 0; I <= 0x1040; I += 8){
        encrypt64(scratch.data());      //;encrypt S (64bit) by keybuf
        *(Word*)(keybuf.data()+I+0) = *(Word*)(scratch.data()+4);    //;write S to keybuf (first upper 32bit)
        *(Word*)(keybuf.data()+I+4) = *(Word*)(scratch.data()+0);    //;write S to keybuf (then lower 32bit)
    }
}

void nds::Cartridge::initKeycode(Word idcode, Word level, Word modulo)
{
    keybuf = {};
    keycode = {};
    std::memcpy(keybuf.data(), bus->bus7->bios7.data() + 0x30, 0x1048);
    *(Word*)(keycode.data()+0)=idcode;
    *(Word*)(keycode.data()+4)=idcode/2;
    *(Word*)(keycode.data()+8)=idcode*2;
    if(level>=1) applyKeycode(modulo);                  // ;first apply (always)
    if(level>=2) applyKeycode(modulo);                 //;second apply (optional)
    *(Word*)(keycode.data()+4) *= 2;
    *(Word*)(keycode.data()+8) /= 2;
    if(level>=3) applyKeycode(modulo);                   // ;third apply (optional)
}

