#pragma once
#include <string>
#include <vector>
#include <cstdint>
#include <memory>
#ifdef BUILD_WEB    
#include <emscripten.h>
#include <emscripten/bind.h>
using namespace emscripten;
#endif

class Console{
    protected:
    std::string loadedGame = "";
    
    //Helfer
    std::string ihex(uintptr_t input);
    std::string ihexNorm(std::string s, int n);

    // Macht das ganze einfacher, weil ich einen Callback weniger in JS brauche
    float volume = 1.0f;
    
    public:
    Console() = default;
    Console(const char* path);
    virtual ~Console() = default;

    virtual void load(const char* path) = 0;
    virtual void clock() = 0;
    virtual void clockUntilSampleReady() = 0;
    virtual const uint8_t* accessFramebuffer() = 0;
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
    std::string getGameTitle();
    void setName(std::string name);
    void setVolume(float v);

    virtual bool canSave() = 0;
    virtual std::vector<uint8_t> getSaveData() = 0;

    // Bios usw.
    virtual std::vector<std::string> getRequiredFiles() = 0;
    virtual void loadSpecialFile(std::string name, std::vector<uint8_t> content) = 0;



    //Debug
    virtual void addClock() = 0;
    virtual void setHalt(bool val) = 0;
    virtual bool isHalted() = 0;
    virtual void produceDisassembly(bool val) = 0;
    virtual int addressBytes() = 0;
    virtual std::pair<std::string, std::vector<int>> getCurrentDisassembly() = 0;
    virtual std::pair<std::string, std::vector<int>> getOldDisassembly() = 0;
    virtual std::vector<uint64_t> addBreakpoint(uint64_t bp) = 0;
    virtual std::vector<uint64_t> removeBreakpoint(uint64_t bp) = 0;
    virtual std::vector<std::string> addBreakpointOP(std::string bp) = 0;
    virtual std::vector<std::string> removeBreakpointOP(std::string bp) = 0;
    virtual std::string getText(uint64_t addr) = 0;
    virtual std::string getOpcodeName(size_t index) = 0;
    virtual uint8_t readCpuBus(uint64_t addr) = 0;

    virtual void displayRegisters() = 0;

    #ifdef BUILD_WEB
      val accessFramebufferJS() {
          return val( typed_memory_view(this->getX() * this->getY() * 4, this->accessFramebuffer()));
      }
    #endif

};


void createConsole(const char* path);

std::unique_ptr<Console> createConsoleFromData(std::string filename, std::vector<uint8_t> rom);


