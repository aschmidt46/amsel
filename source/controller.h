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

    std::shared_ptr<Mapper> mapper = nullptr;

    public:

    ControllerState state;

    void init(std::shared_ptr<Mapper> m){
        mapper = m;
    };

    void setKey(int key, int v);
    void clock();
};