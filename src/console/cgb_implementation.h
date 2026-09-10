#pragma once
#include "cgb_bridge.h"
#include "console.h"
#include "framework/global.h"
#include <iostream>
#include <memory>
#include <optional>
#include <system_error>
#ifdef BUILD_LIBRETRO_CORE
#include <mutex>
#endif

// Rust FFI, wird während build generiert
#include "rusty_bridge/bridge.h"

class CgbImplementation : public Console, std::enable_shared_from_this<CgbImplementation>{
    private:
    std::shared_ptr<NetworkState> netState;
    rust::Box<CGB> cgb;
    static std::vector<SystemOption> options;
    // Retroarch verwendet offenbar (im Audio?) eine Art von Nebenläufigkeit, die dafür sorgt, dass mehrere Funktionen auf dem Console Objekt gleichzeitig aufgerufen werden (können).
    // Das führt zu einem Laufzeitfehler in Rust, weil dadurch das CGB Objekt mehrfach geborrowed wird -> panic
    // Der Mutex ist ein einfacher Workaround
    // #ifdef BUILD_LIBRETRO_CORE
    std::mutex m;
    // #endif
    void setAddressOf(int i, int to);

    void clockLinkCable();
    public:
    std::string getConsoleName() override;
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
    void renderCustomMenuDesktop() override;
    void transferReceive(uint8_t value);
    void transferSent();

    std::vector<SystemOption>* getSystemOptions() override;

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

