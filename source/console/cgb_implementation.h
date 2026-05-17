#pragma once
#include "console.h"

// Rust FFI, wird während build generiert
#include "rusty_bridge/bridge.h"

class CgbImplementation : public Console{
    private:
    rust::Box<CGB> cgb;
    void setAddressOf(int i, int to);
    public:
    CgbImplementation() = default;
    CgbImplementation(const char* path);
    ~CgbImplementation();
    void load(const char* path) override;
    void clock() override;
    void clockUntilSampleReady() override;
    const float* accessFramebuffer() override;
    bool frameIsReady() override;
    bool audioSampleReady() override;
    std::pair<double, double> getSample() override;
    bool isLoaded() override;
    float getX() override;
    float getY() override;
    void setController1Key(bool gamepad, int key, int action) override;
    void setController2Key(bool gamepad, int key, int action) override;


    void addClock() override;
    void setHalt(bool val) override;
    bool isHalted() override;
    void produceDisassembly(bool val) override;
    std::pair<std::string, std::vector<int>> getCurrentDisassembly() override;
    std::pair<std::string, std::vector<int>> getOldDisassembly() override;
    std::vector<uint16_t> addBreakpoint(uint16_t bp) override;
    std::vector<uint16_t> removeBreakpoint(uint16_t bp) override;
    std::vector<std::string> addBreakpointOP(std::string bp) override;
    std::vector<std::string> removeBreakpointOP(std::string bp) override;
    std::string getText(uint16_t addr) override;
    std::string getOpcodeName(size_t index) override;
    uint8_t readCpuBus(uint16_t addr) override;

    void displayRegisters() override;
};

