#pragma once
#include <cstdint>
#include <memory>

class Mapper;

class AbstractMapper{
    public:
    std::shared_ptr<Mapper> mapper;
    virtual void writeRam(uint8_t* addr, uint8_t value) = 0;
};