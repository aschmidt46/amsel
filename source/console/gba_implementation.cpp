#include "gba_implementation.h"
#include "framework/global.h"
#include "framework/stringlib.h"
#ifdef BUILD_DESKTOP
#include <imgui.h>
#endif
#include "../framework/stringlib.h"
//           0  1   2     3      4     5    6    7    8  9
// KEYINPUT: A, B, SEL, START, Right, Left, Up, Down, R, L
// Address:  UpDwn Lft   Rgt    A      B   Sta  Sel   -  -
constexpr int translateKeypad[16] = {6, 7, 5, 4, 0, 1, 3, 2, 9, 8, 0, 0, 0, 0, 0, 0};

void GbaImplementation::setAddressOf(int i, int to){

    if(to){
        gba->press(translateKeypad[i]);
    }
    else{
        gba->release(translateKeypad[i]);
    }
}

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

const uint8_t *GbaImplementation::accessFramebuffer()
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

void GbaImplementation::setController1Key(bool gamepad, int key, int action) {
    (void)gamepad; (void)key; (void)action;
    auto c = &globalConfig.controller1;
    if (gamepad)
    {
      for (int i = 0; i < 8; i++)
      {
        if ((*c)[i].second == key)
        {
          setAddressOf(i, action);
          return;
        }
      }
    }
    else
    { // Tastatur
      for (int i = 0; i < 8; i++)
      {
        if ((*c)[i].first == key)
        {
          setAddressOf(i, action);
          return;
        }
      }
    }
}

void GbaImplementation::setController2Key(bool gamepad, int key, int action) {
    (void)gamepad; (void)key; (void)action;
}

bool GbaImplementation::canSave()
{
    return false;
}

std::vector<uint8_t> GbaImplementation::getSaveData()
{
    return std::vector<uint8_t>();
}

std::vector<std::string> GbaImplementation::getRequiredFiles() {
  return {"GBA Bios"};
}

void GbaImplementation::loadSpecialFile(std::string name, std::vector<uint8_t> content) {
    if(name == std::string("GBA Bios")){
        gba->loadBios(content);
    }
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

void GbaImplementation::produceDisassembly(bool val) {
    (void)val;
}

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
    (void)addr;
    return std::string();
}

std::string GbaImplementation::getOpcodeName(size_t index)
{
    return gba->getDisassembly(index);
}

uint8_t GbaImplementation::readCpuBus(uint64_t addr)
{
    return gba->readBus(addr);
}



void GbaImplementation::displayRegisters() {
    #ifdef BUILD_DESKTOP
    gba::CpuRegisterState state = gba->getRegs();

    ImGui::BeginTable("Register", 2);
    ImGui::TableNextColumn();
    
    for(int i = 0; i < 16; i++){
        ImGui::Text("%s", ("R"+std::to_string(i)+":\t\t"+getHex0x(state.R[i],8)).c_str());
    }
    ImGui::NewLine();
    ImGui::NewLine();
    ImGui::Text("%s", (std::string("Mode: ")+gba->getMode()).c_str());

    auto transaction = gba->getLastTransaction();

    ImGui::Text("%s", (locale.getTranslation(LastMemoryTransaction) + " " + std::get<0>(transaction)).c_str());
    ImGui::Text("%s", ("Addr: " + std::get<1>(transaction)).c_str());
    ImGui::Text("%s", ("Data: " + std::get<2>(transaction)).c_str());
    ImGui::TableNextColumn();

    for(int i = 0; i < 7; i++){
        ImGui::Text("%s", ("R"+std::to_string(i+8)+"_fiq: \t"+getHex0x(state.R_fiq[i],8)).c_str());
    }
    for(int i = 0; i < 2; i++){
        ImGui::Text("%s", ("R"+std::to_string(i+13)+"_svc:\t"+getHex0x(state.R_svc[i],8)).c_str());
    }
    for(int i = 0; i < 2; i++){
        ImGui::Text("%s", ("R"+std::to_string(i+13)+"_abt:\t"+getHex0x(state.R_abt[i],8)).c_str());
    }
    for(int i = 0; i < 2; i++){
        ImGui::Text("%s", ("R"+std::to_string(i+13)+"_irq:\t"+getHex0x(state.R_irq[i],8)).c_str());
    }
    for(int i = 0; i < 2; i++){
        ImGui::Text("%s", ("R"+std::to_string(i+13)+"_und:\t"+getHex0x(state.R_und[i],8)).c_str());
    }
    ImGui::Text("%s", ("CPSR: \t\t"+getHex0x(state.CPSR, 8)).c_str());
    ImGui::Text("%s", ("SPSR_fiq:\t"+getHex0x(state.SPSR[0],8)).c_str());
    ImGui::Text("%s", ("SPSR_svc:\t"+getHex0x(state.SPSR[1],8)).c_str());
    ImGui::Text("%s", ("SPSR_abt:\t"+getHex0x(state.SPSR[2],8)).c_str());
    ImGui::Text("%s", ("SPSR_irq:\t"+getHex0x(state.SPSR[3],8)).c_str());
    ImGui::Text("%s", ("SPSR_und:\t"+getHex0x(state.SPSR[4],8)).c_str());
    ImGui::Text("%s", ("Pipeline[0]:\t"+getHex0x(state.Pipeline[0],8)).c_str());
    ImGui::Text("%s", ("Pipeline[1]:\t"+getHex0x(state.Pipeline[1],8)).c_str());
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
