#include "nds.h"

using namespace nds;

nds::NDS::NDS(const std::vector<uint8_t> &bytes)
{
    bus = std::make_shared<SharedBus>();
}

uint8_t *nds::NDS::accessFramebuffer()
{
    return nullptr;
}

void nds::NDS::clock()
{
    bus->clock();

    audioTime += audioTimePerGBAClock;
    if(audioTime >= audioTimePerSystemSample){
        audioTime -= audioTimePerSystemSample;
        auto [l, r] = bus->getSample();
        audioSampleL = l;
        audioSampleR = r;
        audioSampleReady = true;
    }
}

void nds::NDS::clockUntilSampleReady()
{
    while(!hasSample()){
        clock();
    }
}

bool nds::NDS::hasFrame()
{
    return false;
}

bool nds::NDS::hasSample()
{
    bool tmp = audioSampleReady;
    audioSampleReady = false;
    return tmp;
}

bool nds::NDS::canSave()
{
    return false;
}

std::vector<uint8_t> nds::NDS::getSaveData()
{
    return std::vector<uint8_t>();
}

size_t nds::NDS::getSaveSize()
{
    return size_t();
}

void nds::NDS::loadSave(const std::vector<uint8_t> &saveData)
{
}

std::pair<float, float> nds::NDS::getSample()
{
    return {audioSampleL, audioSampleR};
}

void nds::NDS::press(int i)
{
}

void nds::NDS::release(int i)
{
}

void nds::NDS::setDebug(bool to)
{
}

void nds::NDS::setHalt(bool to)
{
}

bool nds::NDS::isHalted()
{
    return false;
}

void nds::NDS::addClock()
{
}

std::pair<std::string, std::vector<int>> nds::NDS::getNextInstructions()
{
    return std::pair<std::string, std::vector<int>>();
}

std::pair<std::string, std::vector<int>> nds::NDS::getPrevInstructions()
{
    return std::pair<std::string, std::vector<int>>();
}

std::vector<std::string> nds::NDS::removeBreakpointOP(std::string bp)
{
    return std::vector<std::string>();
}

std::vector<std::string> nds::NDS::addBreakpointOP(std::string bp)
{
    return std::vector<std::string>();
}

CpuRegisterState nds::NDS::getRegs()
{
    return CpuRegisterState();
}

std::vector<std::string> nds::NDS::getStack()
{
    return std::vector<std::string>();
}

std::string nds::NDS::getMode()
{
    return std::string();
}

std::string nds::NDS::getState()
{
    return std::string();
}

std::vector<uint64_t> nds::NDS::addBreakpoint(uint64_t bp)
{
    return std::vector<uint64_t>();
}

std::vector<uint64_t> nds::NDS::removeBreakpoint(uint64_t bp)
{
    return std::vector<uint64_t>();
}

std::string nds::NDS::getDisassembly(uint64_t code)
{
    return std::string();
}

uint64_t nds::NDS::readBus(uint64_t addr)
{
    return 0;
}

std::tuple<std::string, std::string, std::string> nds::NDS::getLastTransaction()
{
    return std::tuple<std::string, std::string, std::string>();
}
