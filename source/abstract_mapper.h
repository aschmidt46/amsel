#pragma once
#include <cstdint>
#include <memory>

class Mapper;

class AbstractMapper{
    public:
    std::shared_ptr<Mapper> mapper;
    virtual void writeRam(uint8_t* addr, uint8_t value) = 0;
    virtual uint8_t readRam(uint8_t* addr) = 0;
};