#include "nes_implementation.h"
#ifdef BUILD_DESKTOP
#include <imgui.h>
#include <bitset>
#include "../framework/stringlib.h"
#endif
#include "framework/common.h"

std::string NesImplementation::getConsoleName(){
    return "Nintendo Entertainment System";
}

NesImplementation::NesImplementation(std::vector<uint8_t> &rom)
{
    new (&this->console) NES();
    this->console.load(rom);
}

void NesImplementation::load(const char *path)
{
    new (&this->console) NES();
    this->console.load(path);
}

void NesImplementation::clock()
{
    this->console.clock();
}

void NesImplementation::clockUntilSampleReady()
{
    while (!this->console.hasAudioSample())
    {
        this->console.clock();
        if (console.halt)
            break;
    }
}

const uint8_t *NesImplementation::accessFramebuffer()
{
    return this->console.accessFramebuffer();
}

bool NesImplementation::frameIsReady()
{
    return this->console.frameIsReady();
}

bool NesImplementation::audioSampleReady()
{
    return this->console.hasAudioSample();
}

std::pair<double, double> NesImplementation::getSample()
{
    double sample = this->volume * this->console.getSample();
    return {sample, sample};
}

bool NesImplementation::isLoaded()
{
    return this->console.isLoaded();
}

float NesImplementation::getX()
{
    return this->console.getX();
}

float NesImplementation::getY()
{
    return this->console.getY();
}

void NesImplementation::setKey(int controller, AmselControlAction key, int action)
{
    if(controller == 0){
        this->console.setController1Key(key, action);
    }
    else{
        this->console.setController2Key(key, action);
    }
}

bool NesImplementation::canSave()
{
    return this->console.canSave();
}

std::vector<uint8_t> NesImplementation::getSaveData()
{
    return this->console.getSaveData();
}

std::vector<std::string> NesImplementation::getRequiredFiles() {
  return std::vector<std::string>();
}

void NesImplementation::loadSpecialFile(std::string name, std::vector<uint8_t> content) {
    
}

std::vector<SystemOption>* NesImplementation::getSystemOptions(){
    return &options;
}

ControlOptions *NesImplementation::getControlOptions()
{
    return &controls;
}

void NesImplementation::initializeControls()
{
    controls.controllers = {
        std::vector<std::pair<int,int>>(defaultController1.begin(), defaultController1.begin() + 8),
        std::vector<std::pair<int,int>>(defaultController2.begin(), defaultController2.begin() + 8)
    };
    controls.mapping = std::vector<AmselControlAction>(defaultMapping.begin(), defaultMapping.begin() + 8);
    Console::loadMappings();
}

void NesImplementation::renderCustomMenuDesktop(){

}

void NesImplementation::addClock()
{
    this->console.allowedClocks = 1;
}

void NesImplementation::setHalt(bool val)
{
    this->console.halt = val;
}

bool NesImplementation::isHalted()
{
    return this->console.halt;
}

void NesImplementation::produceDisassembly(bool val)
{
    this->console.produceDisassembly = val;
}

int NesImplementation::addressBytes() {
    return 2;
}

std::pair<std::string, std::vector<int>> NesImplementation::getCurrentDisassembly()
{
    return this->console.getCurrentDisassembly();
}

std::pair<std::string, std::vector<int>> NesImplementation::getOldDisassembly()
{
    return this->console.getOldDisassembly();
}

std::vector<uint64_t> NesImplementation::addBreakpoint(uint64_t bp)
{
    return this->console.addBreakpoint(bp);
}

std::vector<uint64_t> NesImplementation::removeBreakpoint(uint64_t bp)
{
    return this->console.removeBreakpoint(bp);
}

std::vector<std::string> NesImplementation::addBreakpointOP(std::string bp)
{
    return this->console.addBreakpointOP(bp);
}

std::vector<std::string> NesImplementation::removeBreakpointOP(std::string bp)
{
    return this->console.removeBreakpointOP(bp);
}

std::string NesImplementation::getText(uint64_t addr)
{
    return this->console.getText(addr);
}

std::string NesImplementation::getOpcodeName(size_t index)
{
    return this->console.getOpcodeName(index);
}

uint8_t NesImplementation::readCpuBus(uint64_t addr)
{
    return this->console.readCpuBus(addr);
}

void NesImplementation::displayRegisters()
{
    #ifdef BUILD_DESKTOP
    ImGui::BeginTable("Register", 2);
    ImGui::TableNextColumn();
    ImGui::Text("%s", ("P: " + std::bitset<8>(console.readRegister(CpuReg::RegP)).to_string()).c_str());
    ImGui::Text("%s", ("PC: " + getHexDollar(console.readRegister(CpuReg::RegPC), 4)).c_str());
    ImGui::Text("%s", ("SP: " + getHexDollar(console.readRegister(CpuReg::RegSP), 2)).c_str());
    ImGui::TableNextColumn();
    ImGui::Text("%s", ("A: " + getHexDollar(console.readRegister(CpuReg::RegA), 2)).c_str());
    ImGui::Text("%s", ("X: " + getHexDollar(console.readRegister(CpuReg::RegX), 2)).c_str());
    ImGui::Text("%s", ("Y: " + getHexDollar(console.readRegister(CpuReg::RegY), 2)).c_str());
    ImGui::EndTable();
    #endif
}

std::vector<SystemOption> NesImplementation::options = {};

#ifdef FEATURE_LOCAL_MULTIPLAYER
    #define NUM_CONTROLLERS 2
#else
    #define NUM_CONTROLLERS 1
#endif

ControlOptions NesImplementation::controls = {
    .numControllers = NUM_CONTROLLERS,
    .numButtons = 8,
    .controllers = {},
    .mapping = {},
    .joysticks = {0, 1}
};
