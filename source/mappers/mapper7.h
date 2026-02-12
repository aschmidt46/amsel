#pragma once

#include "../abstract_mapper.h"
#include "../mapper.h"

class Mapper7 : AbstractMapper {
    uint8_t bankSelect = 0;


    public:
    void writeRam(uint8_t* addr, uint8_t value) override;
    uint8_t readRam(uint8_t* addr) override;

    Mapper7(std::shared_ptr<Mapper> m);
    void reset() override;

};
