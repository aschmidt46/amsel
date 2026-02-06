#pragma once

#include "../abstract_mapper.h"

class Mapper0 : AbstractMapper {
    public:
    void writeRam(uint8_t* addr, uint8_t value) override {
        return;
    }

    Mapper0(Mapper* m){
        this->mapper = m;
    }

};
