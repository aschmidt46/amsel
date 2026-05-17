#pragma once
#include <string>
#include <vector>
#include <cstdint>

class Console{
    protected:
    std::string loadedGame = "";
    public:

    //Helfer
    std::string ihex(uintptr_t input);
    std::string ihexNorm(std::string s, int n);

    Console() = default;
    Console(const char* path);
    virtual ~Console() = default;

    virtual void load(const char* path) = 0;
    virtual void clock() = 0;
    virtual void clockUntilSampleReady() = 0;
    virtual const float* accessFramebuffer() = 0;
    virtual bool frameIsReady() = 0;
    virtual bool audioSampleReady() = 0;
    // Stereo sample
    virtual std::pair<double, double> getSample() = 0;
    virtual bool isLoaded() = 0;
    // Feste Bildschirmbreite
    virtual float getX() = 0;
    // Feste Bildschirmhöhe
    virtual float getY() = 0;
    virtual void setController1Key(bool gamepad, int key, int action) = 0;
    virtual void setController2Key(bool gamepad, int key, int action) = 0;



    //Debug
    virtual void addClock() = 0;
    virtual void setHalt(bool val) = 0;
    virtual bool isHalted() = 0;
    virtual void produceDisassembly(bool val) = 0;
    virtual std::pair<std::string, std::vector<int>> getCurrentDisassembly() = 0;
    virtual std::pair<std::string, std::vector<int>> getOldDisassembly() = 0;
    virtual std::vector<uint16_t> addBreakpoint(uint16_t bp) = 0;
    virtual std::vector<uint16_t> removeBreakpoint(uint16_t bp) = 0;
    virtual std::vector<std::string> addBreakpointOP(std::string bp) = 0;
    virtual std::vector<std::string> removeBreakpointOP(std::string bp) = 0;
    virtual std::string getText(uint16_t addr) = 0;
    virtual std::string getOpcodeName(size_t index) = 0;
    virtual uint8_t readCpuBus(uint16_t addr) = 0;

    virtual void displayRegisters() = 0;
};

void createConsole(const char* path);

