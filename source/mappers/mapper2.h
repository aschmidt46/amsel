#pragma once

#include "../abstract_mapper.h"
#include "../mapper.h"

class Mapper2 : AbstractMapper {
    public:
    void writeRam(uint8_t* addr, uint8_t value) override;

    Mapper2(Mapper* m);

};
