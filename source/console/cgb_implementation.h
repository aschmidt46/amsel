#pragma once
#include "console.h"
#include <mutex>

// Rust FFI, wird während build generiert
#include "rusty_bridge/bridge.h"

class CgbImplementation : public Console{
    private:
    rust::Box<CGB> cgb;
    #ifdef BUILD_LIBRETRO_CORE
    std::mutex m;
    #endif
    void setAddressOf(int i, int to);
    public:
    CgbImplementation() = delete;
    CgbImplementation(const char* path);
    CgbImplementation(std::vector<uint8_t> &rom);
    ~CgbImplementation();
    void load(const char* path) override;
    void clock() override;
    void clockUntilSampleReady() override;
    const uint8_t* accessFramebuffer() override;
    bool frameIsReady() override;
    bool audioSampleReady() override;
    std::pair<double, double> getSample() override;
    bool isLoaded() override;
    float getX() override;
    float getY() override;
    void setController1Key(bool gamepad, int key, int action) override;
    void setController2Key(bool gamepad, int key, int action) override;

    std::vector<std::string> getRequiredFiles() override;
    void loadSpecialFile(std::string name, std::vector<uint8_t> content) override;

    bool canSave() override;
    std::vector<uint8_t> getSaveData() override;


    void addClock() override;
    void setHalt(bool val) override;
    bool isHalted() override;
    void produceDisassembly(bool val) override;
    int addressBytes() override;
    std::pair<std::string, std::vector<int>> getCurrentDisassembly() override;
    std::pair<std::string, std::vector<int>> getOldDisassembly() override;
    std::vector<uint64_t> addBreakpoint(uint64_t bp) override;
    std::vector<uint64_t> removeBreakpoint(uint64_t bp) override;
    std::vector<std::string> addBreakpointOP(std::string bp) override;
    std::vector<std::string> removeBreakpointOP(std::string bp) override;
    std::string getText(uint64_t addr) override;
    std::string getOpcodeName(size_t index) override;
    uint8_t readCpuBus(uint64_t addr) override;

    void displayRegisters() override;
};

