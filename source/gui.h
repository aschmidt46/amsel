#pragma once
#include "nes.h"


class NES;
class Gui{
    NES* console;
    bool show = true;

    public:
    Gui(NES* c){
        this->console = c;
    };
    void render();
};