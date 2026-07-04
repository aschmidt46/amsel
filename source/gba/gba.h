#pragma once
#include "arm/arm7tdmi_types.h"
#include "bus.h"

namespace gba{
    class GBA{
        std::shared_ptr<Bus> bus;

        double audioTime = 0;
        const double sampleRate = 20000;
        const double audioTimePerGBAClock = 1.0 / 16777918.08;
        const double audioTimePerSystemSample = 1.0 / sampleRate;
        double audioSampleL = 0;
        double audioSampleR = 0;
        bool audioSampleReady = false;


        public:
        GBA(const char* path);
        GBA(const std::vector<uint8_t> &bytes);

        uint8_t* accessFramebuffer();
        void clock();
        void clockUntilSampleReady();
        bool hasFrame();
        bool hasSample();
        std::pair<float, float> getSample();

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
        std::vector<uint64_t> addBreakpoint(uint64_t bp);
        std::vector<uint64_t> removeBreakpoint(uint64_t bp);

        std::string getDisassembly(uint64_t code);
        uint64_t readBus(uint64_t addr);
        std::tuple<std::string, std::string, std::string> getLastTransaction();
    };
}
