#pragma once

#include "abstract_mapper.h"

class Mapper0 : public AbstractMapper {
    public:
    void writeRam(uint8_t* addr, uint8_t value) override;
    uint8_t readRam(uint8_t* addr) override;

    Mapper0(std::shared_ptr<Mapper> m) : AbstractMapper(m) {};

};
