#pragma once
#include <cstdint>
#include <memory>

enum Mirror{
    MIRROR_VERTICAL,
    MIRROR_HORIZONTAL
};

class Mapper;

class AbstractMapper{
    public:
    std::shared_ptr<Mapper> mapper;

    // Diese beiden Funktionen werden nur im Adressraum >= 0x8000 aufgerufen
    virtual void writeRam(uint8_t* addr, uint8_t value) = 0;
    virtual uint8_t readRam(uint8_t* addr) = 0;

    virtual void writePPU(uint8_t* addr, uint8_t value);
    virtual uint8_t readPPU(uint8_t* addr);
};