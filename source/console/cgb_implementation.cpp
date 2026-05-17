#include "cgb_implementation.h"
#include "../framework/global.h"
#include <cstring>
#include <iostream>
#include <imgui.h>
#include <bitset>

void CgbImplementation::setAddressOf(int i, int to)
{
  switch (i)
  {
  case 0:
  { // hoch
    if (to)
    {
      press_joypad(cgb, 2);
    }
    else
    {
      release_joypad(cgb, 2);
    }
    break;
  }
  case 1:
  { // runter
    if (to)
    {
      press_joypad(cgb, 3);
    }
    else
    {
      release_joypad(cgb, 3);
    }
    break;
  }
  case 2:
  { // links
    if (to)
    {
      press_joypad(cgb, 1);
    }
    else
    {
      release_joypad(cgb, 1);
    }
    break;
  }
  case 3:
  { // rechts
    if (to)
    {
      press_joypad(cgb, 0);
    }
    else
    {
      release_joypad(cgb, 0);
    }
    break;
  }
  case 4:
  { // a
    if (to)
    {
      press_button(cgb, 0);
    }
    else
    {
      release_button(cgb, 0);
    }
    break;
  }
  case 5:
  { // b
    if (to)
    {
      press_button(cgb, 1);
    }
    else
    {
      release_button(cgb, 1);
    }
    break;
  }
  case 6:
  { // start
    if (to)
    {
      press_button(cgb, 3);
    }
    else
    {
      release_button(cgb, 3);
    }
    break;
  }
  case 7:
  { // select
    if (to)
    {
      press_button(cgb, 2);
    }
    else
    {
      release_button(cgb, 2);
    }
    break;
  }
  }
}

CgbImplementation::CgbImplementation(const char *path) : cgb(new_cgb(path))
{
}

void CgbImplementation::load(const char *path)
{
  cgb = new_cgb(path);
}

void CgbImplementation::clock()
{
  cgb_clock(cgb);
}

void CgbImplementation::clockUntilSampleReady()
{
  cgb_clock_until_samle_ready(cgb);
}

const float *CgbImplementation::accessFramebuffer()
{
  return access_framebuffer(cgb);
}

bool CgbImplementation::frameIsReady()
{
  return has_frame(cgb);
}

bool CgbImplementation::audioSampleReady()
{
  return audio_sample_ready(cgb);
}

std::pair<double, double> CgbImplementation::getSample()
{
  auto res = get_stereo(cgb);
  return {res.left, res.right};
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

void CgbImplementation::setController1Key(bool gamepad, int key, int action)
{
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

void CgbImplementation::setController2Key(bool gamepad, int key, int action)
{
  // this->setController1Key(gamepad, key, action);
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

std::vector<uint16_t> CgbImplementation::addBreakpoint(uint16_t bp)
{
  std::lock_guard lock(consoleLock);
  auto bps = add_breakpoint(cgb, bp);
  auto res = std::vector<uint16_t>();
  for(const auto &el : bps){
    res.push_back(el);
  }
  return res;
}

std::vector<uint16_t> CgbImplementation::removeBreakpoint(uint16_t bp)
{
  std::lock_guard lock(consoleLock);
  auto bps = remove_breakpoint(cgb, bp);
  auto res = std::vector<uint16_t>();
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

std::string CgbImplementation::getText(uint16_t addr)
{
  return std::string();
}

std::string CgbImplementation::getOpcodeName(size_t index)
{
  std::lock_guard lock(consoleLock);
  return std::string(get_mnemonic(cgb, index));
}

uint8_t CgbImplementation::readCpuBus(uint16_t addr)
{
  std::lock_guard lock(consoleLock);
  return cgb_read_cpu(cgb, addr);
}

void CgbImplementation::displayRegisters()
{
  std::lock_guard lock(consoleLock);
  ImGui::BeginTable("Register", 2);
  ImGui::TableNextColumn();
  ImGui::Text(("F: " + std::bitset<8>(cgb_read_register_8(cgb, CGBRegister8::F)).to_string()).c_str());
  ImGui::Text(("PC: $" + ihexNorm(ihex(cgb_read_register_16(cgb, CGBRegister16::PC)), 4)).c_str());
  ImGui::Text(("SP: $" + ihexNorm(ihex(cgb_read_register_16(cgb, CGBRegister16::SP)), 2)).c_str());
  ImGui::Text(("D: $" + ihexNorm(ihex(cgb_read_register_8(cgb, CGBRegister8::D)), 2)).c_str());
  ImGui::Text(("H: $" + ihexNorm(ihex(cgb_read_register_8(cgb, CGBRegister8::H)), 2)).c_str());
  ImGui::TableNextColumn();
  ImGui::Text(("A: $" + ihexNorm(ihex(cgb_read_register_8(cgb, CGBRegister8::A)), 2)).c_str());
  ImGui::Text(("B: $" + ihexNorm(ihex(cgb_read_register_8(cgb, CGBRegister8::B)), 2)).c_str());
  ImGui::Text(("C: $" + ihexNorm(ihex(cgb_read_register_8(cgb, CGBRegister8::C)), 2)).c_str());
  ImGui::Text(("E: $" + ihexNorm(ihex(cgb_read_register_8(cgb, CGBRegister8::E)), 2)).c_str());
  ImGui::Text(("L: $" + ihexNorm(ihex(cgb_read_register_8(cgb, CGBRegister8::L)), 2)).c_str());
  ImGui::EndTable();
}
