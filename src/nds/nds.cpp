#include "nds.h"
#include "framework/global.h"

using namespace nds;

void nds::NDS::checkBios()
{
    if(bus->firmware.size() == 0 || bus->bus7->getBiosSize() == 0 || bus->bus9->getBiosSize() == 0){
        executionAllowed = false;
        sendMessage = true;
    }
}

nds::NDS::NDS(const std::vector<uint8_t> &bytes, const StartInfo &info)
{
    bus = std::make_shared<SharedBus>();
    bus->init();
    bus->firmware = info.firmware;
    loadBios7(info.bios7);
    loadBios9(info.bios9);
    checkBios();
}

void nds::NDS::loadBios7(const std::vector<uint8_t> &content)
{
    bus->bus7->loadBios(content);
}

void nds::NDS::loadBios9(const std::vector<uint8_t> &content)
{
    bus->bus9->loadBios(content);
}

void nds::NDS::loadFirmware(const std::vector<uint8_t> &content)
{
    bus->firmware = content;
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
    if(sendMessage){
        MessageStruct m = {.type = MT_ERROR, .title = {}, .content = locale.getTranslation(NDSMissingBios)};
        messageQueue.enqueue(m);
        sendMessage = false;
    }
    if(executionAllowed){
        while(!hasSample()){
            clock();
        }
    }
}

bool nds::NDS::hasFrame()
{
    return true;
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
