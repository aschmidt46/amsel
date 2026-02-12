#pragma once
#include "nes.h"


struct SharedState{
    bool show = true;
    bool showDebugger = false;
    bool halt = false;
    bool showOutput = false;
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
    char bpInputBuf[255];
    char opInputBuf[255];
    std::vector<uint16_t> breakpoints;
    std::vector<std::string> breakpointsOP;
    int runningID = 0;

    uint16_t outputStartsAt = 0x6004;
    char outInputBuf[255];

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
    void toggleTestRomOutput(){
        state->showOutput = !state->showOutput;
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
    Gui(NES* c, SharedState* state, bool debug){
        this->console = c;
        this->state = state;
        for(int i = 0; i < 255; i++){
            memInputBuf[i] = 0;
            bpInputBuf[i] = 0;
            opInputBuf[i] = 0;
            outInputBuf[i] = 0;
        }
        if(debug)
            breakpointsOP = console->addBreakpointOP("BRK");
        opInputBuf[0] = 'B'; opInputBuf[1] = 'R'; opInputBuf[2] = 'K'; opInputBuf[3] = '\0';
        outInputBuf[0] = '6'; outInputBuf[0] = '0'; outInputBuf[0] = '0'; outInputBuf[0] = '4'; outInputBuf[0] = '\0';
    };
    void render();
    void assemblyRender();
    void drawRegisters();
    void drawMemoryReader();
    void drawBreakpoints();
    void drawDebugger();
    void drawOutput();

    void printASM(const std::vector<std::pair<std::string, ASMtype>> &v);
    void ASMLine(std::string l, int id, float r, float g, float b);
};