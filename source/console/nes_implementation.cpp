#include "nes_implementation.h"
#include <imgui.h>
#include <bitset>

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

const float *NesImplementation::accessFramebuffer()
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

void NesImplementation::setController1Key(bool gamepad, int key, int action)
{
    this->console.setController1Key(gamepad, key, action);
}

void NesImplementation::setController2Key(bool gamepad, int key, int action)
{
    this->console.setController2Key(gamepad, key, action);
}

bool NesImplementation::canSave()
{
    return this->console.canSave();
}

std::vector<uint8_t> NesImplementation::getSaveData()
{
    return this->console.getSaveData();
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

std::pair<std::string, std::vector<int>> NesImplementation::getCurrentDisassembly()
{
    return this->console.getCurrentDisassembly();
}

std::pair<std::string, std::vector<int>> NesImplementation::getOldDisassembly()
{
    return this->console.getOldDisassembly();
}

std::vector<uint16_t> NesImplementation::addBreakpoint(uint16_t bp)
{
    return this->console.addBreakpoint(bp);
}

std::vector<uint16_t> NesImplementation::removeBreakpoint(uint16_t bp)
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

std::string NesImplementation::getText(uint16_t addr)
{
    return this->console.getText(addr);
}

std::string NesImplementation::getOpcodeName(size_t index)
{
    return this->console.getOpcodeName(index);
}

uint8_t NesImplementation::readCpuBus(uint16_t addr)
{
    return this->console.readCpuBus(addr);
}

void NesImplementation::displayRegisters()
{
    #ifndef BUILD_WEB
    ImGui::BeginTable("Register", 2);
    ImGui::TableNextColumn();
    ImGui::Text(("P: " + std::bitset<8>(console.readRegister(CpuReg::RegP)).to_string()).c_str());
    ImGui::Text(("PC: $" + ihexNorm(ihex(console.readRegister(CpuReg::RegPC)), 4)).c_str());
    ImGui::Text(("SP: $" + ihexNorm(ihex(console.readRegister(CpuReg::RegSP)), 2)).c_str());
    ImGui::TableNextColumn();
    ImGui::Text(("A: $" + ihexNorm(ihex(console.readRegister(CpuReg::RegA)), 2)).c_str());
    ImGui::Text(("X: $" + ihexNorm(ihex(console.readRegister(CpuReg::RegX)), 2)).c_str());
    ImGui::Text(("Y: $" + ihexNorm(ihex(console.readRegister(CpuReg::RegY)), 2)).c_str());
    ImGui::EndTable();
    #endif
}
