#pragma once
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

        float* accessFramebuffer();
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
    };
}
