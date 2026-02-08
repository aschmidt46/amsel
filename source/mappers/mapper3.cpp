#include "mapper3.h"

void Mapper3::writeRam(uint8_t *addr, uint8_t value)
{
    if(mapper->cart->header.CHRROMSize <= 4){
        bankSelect = value & 0b11;
    }
    else{
        bankSelect = value & 0b1111;
    }

    for(int i = 0; i < 0x2000; i++){
        mapper->ppuMap[i] = mapper->cart->chrRom + (bankSelect * 0x2000) + i;
    }
}

uint8_t Mapper3::readRam(uint8_t *addr)
{
    if(mapper->cart->header.PRGROMSize == 1){
        // Gespiegelt
        return mapper->cart->prgRom[((uintptr_t)addr - 0x8000) % 0x4000];
    }
    else{
        //32KiB
        return mapper->cart->prgRom[(uintptr_t)addr - 0x8000];
    }
}

Mapper3::Mapper3(std::shared_ptr<Mapper> m)
{
    this->mapper = m;
    //2KiB
    prgRam = new uint8_t[0x800];
    for(int i = 0; i < 0x2000; i++){
        mapper->memoryMap[0x6000 + i] = prgRam + (i % 0x800);
    }
    //PRG ROM bereits gesetzt in Mapper

    // 32KiB CHR ROM
    // std::cout << mapper->cart->header.CHRROMSize << std::endl;
    // assert(mapper->cart->header.CHRROMSize == 4 || mapper->cart->header.CHRROMSize == 16);

}

Mapper3::~Mapper3()
{
    delete[] prgRam;
}
