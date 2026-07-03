#pragma once
#include "nes/nes.h"

// Eingabeänderung / Steuerungseinstellung (Gui Zustand)
struct InputWaitContext{
    bool wait = false;
    unsigned int controller = 0;
    Action actionToSet = AC_BUTTON_START;
    bool secondary = false;
};

// Gui Zustand
struct SharedState{
    bool show = true;
    bool fullScreen = false;
    bool showDebugger = false;
    bool halt = false;
    bool showOutput = false;
    bool showInput = false;
    bool showAbout = false;
    int controllerContext = 1; // Controller Nummer
    InputWaitContext waitOn;
};

enum ASMtype{
    ASM_REGULAR,
    ASM_JUMP,
    ASM_CURRENT
};

class NES;
class Gui{

    uint8_t lastReadLow = 0;
    uint8_t lastReadHigh = 0;
    uint8_t lastHighLow = 0;
    uint8_t lastHighHigh = 0;
    char memInputBuf[255];
    char bpInputBuf[255];
    char opInputBuf[255];
    std::vector<uint64_t> breakpoints;
    std::vector<std::string> breakpointsOP;
    int runningID = 1;

    long long outputStartsAt = 0x6004;
    char outInputBuf[255];

    void toggleHalt(){
        if(!state->halt){
            state->halt = true;
            console->setHalt(true);
        }
        else{
            state->halt = false;
            console->setHalt(false);
        }
    };
    
    uint64_t getLastRead(){
        return (uint64_t)lastReadLow | ((uint64_t)lastReadHigh << 8) | ((uint64_t)lastHighLow << 16) | ((uint64_t)lastHighHigh << 24);
    };
    
    void assemblyRender();
    void drawRegisters();
    void drawMemoryReader();
    void drawBreakpoints();
    void drawDebugger();
    void drawAbout();
    void drawOutput();
    void drawControlSettings();
    void drawControlSettingsPage(int controller);
    void buttonChangePrompt(int i, int controller, bool secondary);
    
    void printASM(const std::vector<std::pair<std::string, ASMtype>> &v);
    void ASMLine(std::string l, int id, float r, float g, float b);
    
    
    public:
    Gui(SharedState* state);
    
    void render();
    void toggleDebugger();
    void toggleTestRomOutput();
    
    SharedState* state;
};