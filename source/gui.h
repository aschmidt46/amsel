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

    uint8_t lastReadLow = 0;
    uint8_t lastReadHigh = 0;
    char memInputBuf[255];

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
    };

    uint16_t getLastRead(){
        return (uint16_t)lastReadLow | ((uint16_t)lastReadHigh << 8);
    };

    public:
    Gui(NES* c, SharedState* state){
        this->console = c;
        this->state = state;
        for(int i = 0; i < 255; i++){
            memInputBuf[i] = 0;
        }
    };
    void render();
    void assemblyRender();
    void drawRegisters();
    void drawMemoryReader();
    void drawDebugger();
};