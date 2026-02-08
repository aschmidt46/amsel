#pragma once
#include "nes.h"


struct SharedState{
    bool show = true;
    bool showDebugger = false;
};

class NES;
class Gui{
    NES* console;

    SharedState* state;

    void toggleDebugger(){
        if(!state->showDebugger){
            state->showDebugger = true;
            console->produceDisassembly = true;
        }
        else{
            state->showDebugger = false;
            console->produceDisassembly = false;
        }
    };

    public:
    Gui(NES* c, SharedState* state){
        this->console = c;
        this->state = state;
    };
    void render();
};