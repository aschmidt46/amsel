#include "nds_implementation.h"
#include "console.h"
#include "framework/global.h"
#include "framework/stringlib.h"
#ifdef BUILD_DESKTOP
#include "framework/file_io.h"
#include "framework/screen.h"
#include <imgui.h>
#endif
#include "framework/locale.h"

// FIXEN::
//           0  1   2     3      4     5    6    7    8  9
// KEYINPUT: A, B, SEL, START, Right, Left, Up, Down, R, L
// Address:  UpDwn Lft   Rgt    A      B   Sta  Sel   -  -
constexpr int translateKeypad[16] = {6, 7, 5, 4, 0, 1, 3, 2, 9, 8, 0, 0, 0, 0, 0, 0};

void NDSImplementation::setAddressOf(int i, int to){

    if(to){
        nds->press(translateKeypad[i]);
    }
    else{
        nds->release(translateKeypad[i]);
    }
}

std::string NDSImplementation::getConsoleName(){
    return "Nintendo DS";
}

NDSImplementation::NDSImplementation(const char *path) : Console(path)
{
    load(path);

    #ifdef BUILD_DESKTOP
    if(nds->canSave()){
      if(!FileIO::getInstance().createSave(this->loadedGame)){
        size_t size = nds->getSaveSize();
        std::vector<uint8_t> saveData(size);
        FileIO::getInstance().loadSave(this->loadedGame, saveData, size);
        nds->loadSave(saveData);
      }
    }
    #endif
}

NDSImplementation::NDSImplementation(std::vector<uint8_t> &rom)
{
    nds = std::make_shared<nds::NDS>(rom);

    #ifdef BUILD_DESKTOP
    if(nds->canSave()){
      if(!FileIO::getInstance().createSave(this->loadedGame)){
        size_t size = nds->getSaveSize();
        std::vector<uint8_t> saveData(size);
        FileIO::getInstance().loadSave(this->loadedGame, saveData, size);
        nds->loadSave(saveData);
      }
    }
    #endif
}

NDSImplementation::~NDSImplementation()
{
    #ifdef BUILD_DESKTOP
    if(nds->canSave()){
      auto data = nds->getSaveData();
      FileIO::getInstance().saveData(this->loadedGame, data.data(), data.size());
    }
    #endif
}

void NDSImplementation::load(const char *path)
{
    std::filesystem::path p(path);

    std::ifstream stream(path, std::ios::in | std::ios::binary);
    std::vector<uint8_t> contents((std::istreambuf_iterator<char>(stream)),
                                  std::istreambuf_iterator<char>());

    nds = std::make_shared<nds::NDS>(contents);
}

void NDSImplementation::clock() {
    nds->clock();
}

void NDSImplementation::clockUntilSampleReady() {
    nds->clockUntilSampleReady();
}

const uint8_t *NDSImplementation::accessFramebuffer()
{
    return nds->accessFramebuffer();
}

bool NDSImplementation::frameIsReady()
{
    return nds->hasFrame();
}

bool NDSImplementation::audioSampleReady()
{
    return nds->hasSample();
}

std::pair<double, double> NDSImplementation::getSample()
{
    auto [l, r] = nds->getSample();
    return {this->volume * l, this->volume * r};
}

bool NDSImplementation::isLoaded()
{
    return true;
}

float NDSImplementation::getX()
{
    return 256.0f;
}

float NDSImplementation::getY()
{
    // 192 * 2
    return 384.0f;
}

void NDSImplementation::setController1Key(bool gamepad, int key, int action) {
    (void)gamepad; (void)key; (void)action;
    auto c = &globalConfig.controller1;
    if (gamepad)
    {
      for (int i = 0; i < 10; i++)
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
      for (int i = 0; i < 10; i++)
      {
        if ((*c)[i].first == key)
        {
          setAddressOf(i, action);
          return;
        }
      }
    }
}

void NDSImplementation::setController2Key(bool gamepad, int key, int action) {
    (void)gamepad; (void)key; (void)action;
}

bool NDSImplementation::canSave()
{
    return nds->canSave();
}

std::vector<uint8_t> NDSImplementation::getSaveData()
{
    return nds->getSaveData();
}

std::vector<std::string> NDSImplementation::getRequiredFiles() {
  return {};
}

void NDSImplementation::loadSpecialFile(std::string name, std::vector<uint8_t> content) {
    // if(name == std::string("GBA Bios")){
    //     nds->loadBios(content);
    // }
}

std::vector<SystemOption>* NDSImplementation::getSystemOptions(){
    return &options;
}

void NDSImplementation::renderCustomMenuDesktop(){

}

void NDSImplementation::addClock() {
    nds->addClock();
}

void NDSImplementation::setHalt(bool val) {
    nds->setHalt(val);
}

bool NDSImplementation::isHalted()
{
    return nds->isHalted();
}

void NDSImplementation::produceDisassembly(bool val) {
    nds->setDebug(val);
}

int NDSImplementation::addressBytes() {
    return 4;
}

std::pair<std::string, std::vector<int>> NDSImplementation::getCurrentDisassembly()
{
    return nds->getNextInstructions();
}

std::pair<std::string, std::vector<int>> NDSImplementation::getOldDisassembly()
{
    return nds->getPrevInstructions();
}

std::vector<uint64_t> NDSImplementation::addBreakpoint(uint64_t bp)
{
    return nds->addBreakpoint(bp);
}

std::vector<uint64_t> NDSImplementation::removeBreakpoint(uint64_t bp)
{
        return nds->removeBreakpoint(bp);
}

std::vector<std::string> NDSImplementation::addBreakpointOP(std::string bp)
{
    return nds->addBreakpointOP(bp);
}

std::vector<std::string> NDSImplementation::removeBreakpointOP(std::string bp)
{
    return nds->removeBreakpointOP(bp);
}

std::string NDSImplementation::getText(uint64_t addr)
{
    (void)addr;
    return std::string();
}

std::string NDSImplementation::getOpcodeName(size_t index)
{
    return nds->getDisassembly(index);
}

uint8_t NDSImplementation::readCpuBus(uint64_t addr)
{
    return nds->readBus(addr);
}


void NDSImplementation::displayRegisters() {
    #ifdef BUILD_DESKTOP
    nds::CpuRegisterState state = nds->getRegs();

    ImGui::BeginTable("Register", 2);
    ImGui::TableNextColumn();
    
    for(int i = 0; i < 16; i++){
        ImGui::Text("%s", ("R"+std::to_string(i)+":\t\t"+getHex0x(state.R[i],8)).c_str());
    }
    ImGui::NewLine();
    ImGui::NewLine();
    ImGui::Text("%s", (std::string("Mode: ")+nds->getMode()).c_str());
    ImGui::Text("%s", (std::string("State: ")+nds->getState()).c_str());

    auto transaction = nds->getLastTransaction();

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
    auto stack = nds->getStack();
    std::vector<char*> cstrings;
    cstrings.reserve(stack.size());

    for(size_t i = 0; i < stack.size(); ++i)
        cstrings.push_back(const_cast<char*>(stack[i].c_str()));
    int select = 1;
    ImGui::ListBox("Stack", &select, cstrings.data(), stack.size());
    #endif
}

std::vector<SystemOption> NDSImplementation::options = {};
