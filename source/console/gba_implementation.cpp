#include "gba_implementation.h"
#ifndef BUILD_WEB
#include <imgui.h>
#endif
#include <format>
#include <algorithm>

GbaImplementation::GbaImplementation(const char *path)
{
    gba = std::make_shared<gba::GBA>(path);
}

GbaImplementation::GbaImplementation(std::vector<uint8_t> &rom)
{
    gba = std::make_shared<gba::GBA>(rom);
}

void GbaImplementation::load(const char *path)
{
    gba = std::make_shared<gba::GBA>(path);
}

void GbaImplementation::clock() {
    gba->clock();
}

void GbaImplementation::clockUntilSampleReady() {
    gba->clockUntilSampleReady();
}

const float *GbaImplementation::accessFramebuffer()
{
    return gba->accessFramebuffer();
}

bool GbaImplementation::frameIsReady()
{
    return gba->hasFrame();
}

bool GbaImplementation::audioSampleReady()
{
    return gba->hasSample();
}

std::pair<double, double> GbaImplementation::getSample()
{
    return gba->getSample();
}

bool GbaImplementation::isLoaded()
{
    return true;
}

float GbaImplementation::getX()
{
    return 240.0f;
}

float GbaImplementation::getY()
{
    return 160.0f;
}

void GbaImplementation::setController1Key(bool gamepad, int key, int action) {}

void GbaImplementation::setController2Key(bool gamepad, int key, int action) {}

bool GbaImplementation::canSave()
{
    return false;
}

std::vector<uint8_t> GbaImplementation::getSaveData()
{
    return std::vector<uint8_t>();
}

void GbaImplementation::addClock() {
    gba->addClock();
}

void GbaImplementation::setHalt(bool val) {
    gba->setHalt(val);
}

bool GbaImplementation::isHalted()
{
    return gba->isHalted();
}

void GbaImplementation::produceDisassembly(bool val) {}

int GbaImplementation::addressBytes() {
    return 4;
}

std::pair<std::string, std::vector<int>> GbaImplementation::getCurrentDisassembly()
{
    return gba->getNextInstructions();
}

std::pair<std::string, std::vector<int>> GbaImplementation::getOldDisassembly()
{
    return gba->getPrevInstructions();
}

std::vector<uint64_t> GbaImplementation::addBreakpoint(uint64_t bp)
{
    return gba->addBreakpoint(bp);
}

std::vector<uint64_t> GbaImplementation::removeBreakpoint(uint64_t bp)
{
        return gba->removeBreakpoint(bp);
}

std::vector<std::string> GbaImplementation::addBreakpointOP(std::string bp)
{
    return gba->addBreakpointOP(bp);
}

std::vector<std::string> GbaImplementation::removeBreakpointOP(std::string bp)
{
    return gba->removeBreakpointOP(bp);
}

std::string GbaImplementation::getText(uint64_t addr)
{
    return std::string();
}

std::string GbaImplementation::getOpcodeName(size_t index)
{
    return std::string();
}

uint8_t GbaImplementation::readCpuBus(uint64_t addr)
{
    return 0;
}

std::string thex(uintptr_t input)
{
    std::string str = std::format("{:x}", input);
    std::transform(str.begin(), str.end(), str.begin(), ::toupper);
    return str;
}

std::string thexNorm(std::string s, int n)
{
    while ((int)s.size() < n)
        s = "0" + s;
    return s;
}

std::string getHex(size_t input, int length){
    return "0x"+thexNorm(thex(input), length);
}

void GbaImplementation::displayRegisters() {
    #ifndef BUILD_WEB
    gba::CpuRegisterState state = gba->getRegs();

    ImGui::BeginTable("Register", 2);
    ImGui::TableNextColumn();
    
    for(int i = 0; i < 16; i++){
        ImGui::Text(("R"+std::to_string(i)+":\t\t"+getHex(state.R[i],8)).c_str());
    }
    ImGui::Text("");
    ImGui::Text("");
    ImGui::Text((std::string("Mode: ")+gba->getMode()).c_str());
    ImGui::TableNextColumn();

    for(int i = 0; i < 7; i++){
        ImGui::Text(("R"+std::to_string(i+8)+"_fiq: \t"+getHex(state.R_fiq[i],8)).c_str());
    }
    for(int i = 0; i < 2; i++){
        ImGui::Text(("R"+std::to_string(i+13)+"_svc:\t"+getHex(state.R_svc[i],8)).c_str());
    }
    for(int i = 0; i < 2; i++){
        ImGui::Text(("R"+std::to_string(i+13)+"_abt:\t"+getHex(state.R_abt[i],8)).c_str());
    }
    for(int i = 0; i < 2; i++){
        ImGui::Text(("R"+std::to_string(i+13)+"_irq:\t"+getHex(state.R_irq[i],8)).c_str());
    }
    for(int i = 0; i < 2; i++){
        ImGui::Text(("R"+std::to_string(i+13)+"_und:\t"+getHex(state.R_und[i],8)).c_str());
    }
    ImGui::Text(("CPSR: \t\t"+getHex(state.CPSR, 8)).c_str());
    ImGui::Text(("SPSR_fiq:\t"+getHex(state.SPSR[0],8)).c_str());
    ImGui::Text(("SPSR_svc:\t"+getHex(state.SPSR[1],8)).c_str());
    ImGui::Text(("SPSR_abt:\t"+getHex(state.SPSR[2],8)).c_str());
    ImGui::Text(("SPSR_irq:\t"+getHex(state.SPSR[3],8)).c_str());
    ImGui::Text(("SPSR_und:\t"+getHex(state.SPSR[4],8)).c_str());
    ImGui::Text(("Pipeline[0]:\t"+getHex(state.Pipeline[0],8)).c_str());
    ImGui::Text(("Pipeline[1]:\t"+getHex(state.Pipeline[1],8)).c_str());
    ImGui::EndTable();

    ImGui::Separator();
    auto stack = gba->getStack();
    std::vector<char*> cstrings;
    cstrings.reserve(stack.size());

    for(size_t i = 0; i < stack.size(); ++i)
        cstrings.push_back(const_cast<char*>(stack[i].c_str()));
    int select = 1;
    ImGui::ListBox("Stack", &select, cstrings.data(), stack.size());

    #endif
}
