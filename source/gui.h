#pragma once
#include "nes.h"


struct SharedState{
    bool show = true;
    bool showDebugger = false;
    bool halt = false;
};

enum ASMtype{
    ASM_REGULAR,
    ASM_JUMP,
    ASM_CURRENT
};

class NES;
class Gui{
    NES* console;

    SharedState* state;

    std::vector<std::pair<std::string, ASMtype>> oASM;

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
    void toggleHalt(){
        if(!state->halt){
            state->halt = true;
            console->halt = true;
        }
        else{
            state->halt = false;
            console->halt = false;
        }
    }

    public:
    Gui(NES* c, SharedState* state){
        this->console = c;
        this->state = state;
    };
    void render();
    void assemblyRender();
};