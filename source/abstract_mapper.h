#pragma once
#include <cstdint>

class Mapper;

class AbstractMapper{
    public:
    Mapper* mapper;
    virtual void writeRam(uint8_t* addr, uint8_t value) = 0;
};