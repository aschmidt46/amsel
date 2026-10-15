#pragma once
#include <memory>
#include "bus.h"

namespace nds{

    struct StartInfo{
        std::vector<Byte> bios7;
        std::vector<Byte> bios9;
        std::vector<Byte> firmware;
        std::optional<size_t> haltAt = {};
    };

    const StartInfo defInfo = {};

    class NDS{
        double audioTime = 0;
        const double sampleRate = 20000;
        const double audioTimePerGBAClock = 1.0 / (16777918.08 * 2);
        const double audioTimePerSystemSample = 1.0 / sampleRate;
        double audioSampleL = 0;
        double audioSampleR = 0;
        bool audioSampleReady = false;
        bool executionAllowed = true;
        bool sendMessage = false;

        void checkBios();
    
        public:
        std::shared_ptr<SharedBus> bus;
    
        NDS(const std::vector<uint8_t> &bytes, const StartInfo &info = defInfo);

        void loadBios7(const std::vector<uint8_t> &content);
        void loadBios9(const std::vector<uint8_t> &content);
        void loadFirmware(const std::vector<uint8_t> &content);

        uint8_t* accessFramebuffer();
        void clock();
        void clockUntilSampleReady();
        bool hasFrame();
        bool hasSample();
        bool canSave();
        std::vector<uint8_t> getSaveData();
        size_t getSaveSize();
        void loadSave(const std::vector<uint8_t> &saveData);
        std::pair<float, float> getSample();
        void press(int i);
        void release(int i);


        void setDebug(bool to);
        void setHalt(bool to);
        bool isHalted();
        void addClock();
        std::pair<std::string, std::vector<int>> getNextInstructions();
        std::pair<std::string, std::vector<int>> getPrevInstructions();

        std::vector<std::string> removeBreakpointOP(std::string bp);
        std::vector<std::string> addBreakpointOP(std::string bp);
        CpuRegisterState getRegs();
        std::vector<std::string> getStack();
        std::string getMode();
        std::string getState();
        std::vector<uint64_t> addBreakpoint(uint64_t bp);
        std::vector<uint64_t> removeBreakpoint(uint64_t bp);

        std::string getDisassembly(uint64_t code);
        uint64_t readBus(uint64_t addr);
        std::tuple<std::string, std::string, std::string> getLastTransaction();
    };
}
