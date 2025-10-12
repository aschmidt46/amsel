#pragma once
#include <cstdint>
#include "mapper.h"

union ControllerState{
    uint8_t raw = 0x00;
    struct {
        uint8_t right : 1;
        uint8_t left : 1;
        uint8_t down : 1;
        uint8_t up : 1;
        uint8_t start : 1;
        uint8_t select : 1;
        uint8_t b : 1;
        uint8_t a : 1;
    };
};

class Mapper;
class Controller{

    Mapper* mapper;

    public:

    ControllerState state;

    void init(Mapper* m){
        mapper = m;
    };

    void setKey(int key, int v);
    void clock();
};