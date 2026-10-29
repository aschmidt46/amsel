#include "cgb_implementation.h"
#include "../framework/global.h"
#include "../framework/stringlib.h"
#include "asio/placeholders.hpp"
#include "console.h"
#include "console/cgb_bridge.h"
#include <cstring>
#include <exception>
#include <functional>
#include <limits>
#include <stdexcept>
#ifdef BUILD_DESKTOP
  #include <imgui.h>
  #include "../framework/file_io.h"
#endif
#include <array>
#include <bitset>

// {bit, joypad?}
static std::array<std::pair<int, bool>, 8> actionToInternal = {{
  {2, true}, // up
  {3, true}, //down, usw
  {1, true},
  {0, true},
  {0, false},
  {1, false},
  {3, false},
  {2, false}
}};

std::string CgbImplementation::getConsoleName(){
    #ifdef FEATURE_CGB
    return "Nintendo Gameboy Color";
    #else
    return "Nintendo Gameboy";
    #endif
}

CgbImplementation::CgbImplementation(const char *path) : netState(new NetworkState()), cgb(new_cgb(path, netState)), Console(path)
{
  #ifdef BUILD_DESKTOP
  if(cgb_can_save(cgb)){
    if(!FileIO::getInstance().createSave(this->loadedGame)){
      size_t size = get_save_size(cgb);
      std::vector<uint8_t> saveData(size);
      FileIO::getInstance().loadSave(this->loadedGame, saveData.data(), size);
      cgb_load_save(cgb, saveData);
    }
  }
  #endif
}

CgbImplementation::CgbImplementation(std::vector<uint8_t> &rom) : netState(new NetworkState()), cgb(new_cgb_rom(rom, netState))
{
  #ifdef BUILD_DESKTOP
  if(cgb_can_save(cgb)){
    if(!FileIO::getInstance().createSave(this->loadedGame)){
      size_t size = get_save_size(cgb);
      std::vector<uint8_t> saveData(size);
      FileIO::getInstance().loadSave(this->loadedGame, saveData.data(), size);
      cgb_load_save(cgb, saveData);
    }
  }
  #endif
}

CgbImplementation::~CgbImplementation()
{
  #ifdef BUILD_DESKTOP
  if(cgb_can_save(cgb)){
    auto data = cgb_get_save_data(cgb);
    std::vector<uint8_t> cxxData;
    for(auto el : data){
      cxxData.push_back(el);
    }
    FileIO::getInstance().saveData(this->loadedGame, cxxData.data(), cxxData.size());
  }
  #endif
}

void CgbImplementation::load(const char *path)
{
  cgb = new_cgb(path, netState);

  #ifdef BUILD_DESKTOP
  if(cgb_can_save(cgb)){
    if(!FileIO::getInstance().createSave(this->loadedGame)){
      size_t size = get_save_size(cgb);
      std::vector<uint8_t> saveData(size);
      FileIO::getInstance().loadSave(this->loadedGame, saveData.data(), size);
      cgb_load_save(cgb, saveData);
    }
  }
  #endif
}

void CgbImplementation::clock()
{
  cgb_clock(cgb);
}

void CgbImplementation::clockUntilSampleReady()
{
  // #ifdef BUILD_LIBRETRO_CORE
  std::lock_guard<std::mutex> lock(m);
  // #endif
  cgb_clock_until_samle_ready(cgb);
}

const uint8_t *CgbImplementation::accessFramebuffer()
{
  // #ifdef BUILD_LIBRETRO_CORE
  std::lock_guard<std::mutex> lock(m);
  // #endif
  return (uint8_t*)access_framebuffer(cgb);
}

bool CgbImplementation::frameIsReady()
{
  // #ifdef BUILD_LIBRETRO_CORE
  std::lock_guard<std::mutex> lock(m);
  // #endif
  return has_frame(cgb);
}

bool CgbImplementation::audioSampleReady()
{
  // #ifdef BUILD_LIBRETRO_CORE
  std::lock_guard<std::mutex> lock(m);
  // #endif
  return audio_sample_ready(cgb);
}

std::pair<double, double> CgbImplementation::getSample()
{
  // #ifdef BUILD_LIBRETRO_CORE
  std::lock_guard<std::mutex> lock(m);
  // #endif
  auto res = get_stereo(cgb);
  return {this->volume * res.left, this->volume * res.right};
}

bool CgbImplementation::isLoaded()
{
  return true;
}

float CgbImplementation::getX()
{
  return 160.0f;
}

float CgbImplementation::getY()
{
  return 144.0f;
}

void CgbImplementation::setKey(int controller, AmselControlAction key, int action)
{
  (void)controller;
  // #ifdef BUILD_LIBRETRO_CORE
  std::lock_guard<std::mutex> lock(m);
  // #endif
  auto p = actionToInternal[key];
  if(p.second){
    if(action){
      press_joypad(cgb, p.first);
    }
    else{
      release_joypad(cgb, p.first);
    }
  }
  else{
    if(action){
      press_button(cgb, p.first);
    }
    else{
      release_button(cgb, p.first);
    }
  }
}

std::vector<std::string> CgbImplementation::getRequiredFiles() {
  return std::vector<std::string>();
}

void CgbImplementation::loadSpecialFile(std::string name, std::vector<uint8_t> content) {
  
}

std::vector<SystemOption>* CgbImplementation::getSystemOptions(){
  return &options;
}

ControlOptions *CgbImplementation::getControlOptions()
{
    return &controls;
}

void CgbImplementation::initializeControls()
{
  controls.controllers = {std::vector<std::pair<int,int>>(defaultController1.begin(), defaultController1.begin() + 8)};
  controls.mapping = std::vector<AmselControlAction>(defaultMapping.begin(), defaultMapping.begin() + 8);
  Console::loadMappings();
}

void CgbImplementation::clockLinkCable(){
  // if(connection.has_value()){
  //   connection.value()->writeByte(116);
  //   connection.value()->readByte();
  // }
}

bool CgbImplementation::canSave()
{
    return cgb_can_save(cgb);
}

std::vector<uint8_t> CgbImplementation::getSaveData()
{
    auto data = cgb_get_save_data(cgb);
    std::vector<uint8_t> v;
    for(const auto &el : data){
      v.push_back(el);
    }
    return v;
}

void CgbImplementation::addClock()
{
  std::lock_guard lock(consoleLock);
  cgb_step(cgb);
}

void CgbImplementation::setHalt(bool val)
{
  std::lock_guard lock(consoleLock);
  cgb_set_break(cgb, val);
}

bool CgbImplementation::isHalted()
{
  std::lock_guard lock(consoleLock);
  return cgb_is_halted(cgb);
}

void CgbImplementation::produceDisassembly(bool val)
{
}

int CgbImplementation::addressBytes() {
  return 2;
}

std::pair<std::string, std::vector<int>> CgbImplementation::getCurrentDisassembly()
{
  std::lock_guard lock(consoleLock);
  auto res = get_next_n_instructions(cgb, 10);
  std::string s = "";
  if (res.first.length() > 0)
  {
    s = std::string(res.first);
  }
  std::vector<int> v;
  for (const auto &el : res.second)
  {
    v.push_back(el);
  }
  return {s, v};
}

std::pair<std::string, std::vector<int>> CgbImplementation::getOldDisassembly()
{
  std::lock_guard lock(consoleLock);
  auto res = get_prev_10_instructions(cgb);
  std::string s = "";
  if (res.first.length() > 0)
  {
    s = std::string(res.first);
  }
  std::vector<int> v;
  for (const auto &el : res.second)
  {
    v.push_back(el);
  }
  return {s, v};
}

std::vector<uint64_t> CgbImplementation::addBreakpoint(uint64_t bp)
{
  std::lock_guard lock(consoleLock);
  auto bps = add_breakpoint(cgb, bp);
  auto res = std::vector<uint64_t>();
  for(const auto &el : bps){
    res.push_back(el);
  }
  return res;
}

std::vector<uint64_t> CgbImplementation::removeBreakpoint(uint64_t bp)
{
  std::lock_guard lock(consoleLock);
  auto bps = remove_breakpoint(cgb, bp);
  auto res = std::vector<uint64_t>();
  for(const auto &el : bps){
    res.push_back(el);
  }
  return res;
}

std::vector<std::string> CgbImplementation::addBreakpointOP(std::string bp)
{
  std::lock_guard lock(consoleLock);
  auto bps = add_breakpoint_op(cgb, bp);
  auto res = std::vector<std::string>();
  for(const auto &el : bps){
    res.push_back(std::string(el));
  }
  return res;
}

std::vector<std::string> CgbImplementation::removeBreakpointOP(std::string bp)
{
  std::lock_guard lock(consoleLock);
  auto bps = remove_breakpoint_op(cgb, bp);
  auto res = std::vector<std::string>();
  for(const auto &el : bps){
    res.push_back(std::string(el));
  }
  return res;
}

std::string CgbImplementation::getText(uint64_t addr)
{
  return std::string();
}

std::string CgbImplementation::getOpcodeName(size_t index)
{
  std::lock_guard lock(consoleLock);
  return std::string(get_mnemonic(cgb, index));
}

uint8_t CgbImplementation::readCpuBus(uint64_t addr)
{
  std::lock_guard lock(consoleLock);
  return cgb_read_cpu(cgb, addr);
}

void CgbImplementation::displayRegisters()
{
  #ifdef BUILD_DESKTOP
  std::lock_guard lock(consoleLock);
  ImGui::BeginTable("Register", 2);
  ImGui::TableNextColumn();
  ImGui::Text("%s", ("F: " + std::bitset<8>(cgb_read_register_8(cgb, CGBRegister8::F)).to_string()).c_str());
  ImGui::Text("%s", ("PC: " + getHexDollar(cgb_read_register_16(cgb, CGBRegister16::PC), 4)).c_str());
  ImGui::Text("%s", ("SP: " + getHexDollar(cgb_read_register_16(cgb, CGBRegister16::SP), 2)).c_str());
  ImGui::Text("%s", ("D: " + getHexDollar(cgb_read_register_8(cgb, CGBRegister8::D), 2)).c_str());
  ImGui::Text("%s", ("H: " + getHexDollar(cgb_read_register_8(cgb, CGBRegister8::H), 2)).c_str());
  ImGui::TableNextColumn();
  ImGui::Text("%s", ("A: " + getHexDollar(cgb_read_register_8(cgb, CGBRegister8::A), 2)).c_str());
  ImGui::Text("%s", ("B: " + getHexDollar(cgb_read_register_8(cgb, CGBRegister8::B), 2)).c_str());
  ImGui::Text("%s", ("C: " + getHexDollar(cgb_read_register_8(cgb, CGBRegister8::C), 2)).c_str());
  ImGui::Text("%s", ("E: " + getHexDollar(cgb_read_register_8(cgb, CGBRegister8::E), 2)).c_str());
  ImGui::Text("%s", ("L: " + getHexDollar(cgb_read_register_8(cgb, CGBRegister8::L), 2)).c_str());
  ImGui::EndTable();
  #endif
}

void CgbImplementation::renderCustomMenuDesktop(){
  netState->renderMenuDesktop(this);
}

void CgbImplementation::transferReceive(uint8_t value){
  std::lock_guard<std::mutex> lock(m);
  completed_transfer(cgb, value);
}

void CgbImplementation::transferSent(){
  std::lock_guard<std::mutex> lock(m);
  completed_send(cgb);
}

std::vector<SystemOption> CgbImplementation::options = {CustomMenu{LinkCable}};

ControlOptions CgbImplementation::controls = {
    .numControllers = 1,
    .numButtons = 8,
    .controllers = {
        //std::vector<std::pair<int,int>>(defaultController1.begin(), defaultController1.begin() + 8)
    },
    .mapping = {}//std::vector<AmselControlAction>(defaultMapping.begin(), defaultMapping.begin() + 8)
};
