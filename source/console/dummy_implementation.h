#include "console.h"


class DummyImplementation : public Console{
    private:
    // Ist eigentlich UI Zustand, naja
    bool halt = false;
    public:
    DummyImplementation() = default;
    DummyImplementation(const char* path){};
    ~DummyImplementation() = default;
    void load(const char* path) override{};
    void clock() override{};
    void clockUntilSampleReady() override{};
    const float* accessFramebuffer() override{return nullptr;};
    bool frameIsReady() override{return false;};
    bool audioSampleReady() override{return false;};
    std::pair<double, double> getSample() override{return {};};
    bool isLoaded() override{return false;};
    // Kann nicht 0 sein, weil Textur der Größe 0 in OpenGL nicht legal ist
    float getX() override{return 16.0f;};
    float getY() override{return 16.0f;};
    void setController1Key(bool gamepad, int key, int action) override{};
    void setController2Key(bool gamepad, int key, int action) override{};

    bool canSave() override {return false;};
    std::vector<uint8_t> getSaveData() override {return std::vector<uint8_t>();};


    void addClock() override{};
    void setHalt(bool val) override{halt = val;};
    bool isHalted() override{return halt;};
    void produceDisassembly(bool val) override{};
    int addressBytes() override{return 0;};
    std::pair<std::string, std::vector<int>> getCurrentDisassembly() override{return {};};
    std::pair<std::string, std::vector<int>> getOldDisassembly() override{return {};};
    std::vector<uint64_t> addBreakpoint(uint64_t bp) override{return {};};
    std::vector<uint64_t> removeBreakpoint(uint64_t bp) override{return {};};
    std::vector<std::string> addBreakpointOP(std::string bp) override{return {};};
    std::vector<std::string> removeBreakpointOP(std::string bp) override{return {};};
    std::string getText(uint64_t addr) override{return "";};
    std::string getOpcodeName(size_t index) override{return "";};
    uint8_t readCpuBus(uint64_t addr) override{return 0;};

    void displayRegisters() override{};
};