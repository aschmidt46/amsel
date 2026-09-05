#include "gui.h"

#include <string>
#include <filesystem>
#include <variant>
#include <vector>
#include <bitset>

#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include "console/console.h"
#include "console/gba_implementation.h"
#include "framework/locale.h"
#include "framework/stringlib.h"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "portable-file-dialogs.h"
#define NOTIFY_RENDER_OUTSIDE_MAIN_WINDOW false
#include "ImGuiNotify.hpp"
#include "IconsFontAwesome6.h"

#include "framework/file_io.h"
#include "framework/input.h"
#ifdef BUILD_NES
#include "console/nes_implementation.h"
#endif

#ifdef BUILD_CGB
#include "console/cgb_implementation.h"
#endif
#include "framework/screen.h"

void removeCharsFromString( std::string &str, const char* charsToRemove ) {
   for ( unsigned int i = 0; i < strlen(charsToRemove); ++i ) {
      str.erase( remove(str.begin(), str.end(), charsToRemove[i]), str.end() );
   }
}

constexpr std::string getRomFileList(){
  std::string list = "";
  #ifdef BUILD_NES
  list += "*.nes ";
  #endif
  #ifdef BUILD_CGB
  list += "*.gb *.gbc ";
  #endif
  #ifdef BUILD_GBA
  list += "*.gba ";
  #endif

  return list;
}

std::optional<std::string> openFile(){
  auto fileList = getRomFileList();
  auto result = pfd::open_file(locale.getTranslation(ChooseRom), globalConfig.directory, {(locale.getTranslation(RomFilter) + " " + fileList), fileList}, pfd::opt::none);
  auto res = result.result();
  if(res.size()==0){
    return {};
  }
  std::replace(res[0].begin(), res[0].end(), '\\', '/');
  globalConfig.directory = res[0].substr(0, res[0].find_last_of('/'));
  FileIO::getInstance().saveSettings(globalConfig);
  return res[0];
}

ImGuiToastType mTypeToImGui(MessageType t){
  switch(t){
    case MT_SUCCESS:
      return ImGuiToastType::Success;
    case MT_INFO:
      return ImGuiToastType::Info;
    case MT_WARNING:
      return ImGuiToastType::Warning;
    case MT_ERROR:
      return ImGuiToastType::Error;
    default:
      return ImGuiToastType::Info;
  }
}

const char* getVolumeIcon(float f, bool enabled){
  if(!enabled || f == 0){
    return ICON_FA_VOLUME_XMARK;
  }
  else if(f < 0.37){
    return ICON_FA_VOLUME_OFF;
  }
  else if(f < 0.67){
    return ICON_FA_VOLUME_LOW;
  }
  else{
    return ICON_FA_VOLUME_HIGH;
  }
}

std::string translateGamepadCode(int c){
  // Zuerst Achsen, dann Knöpfe
  int mc = c - 6;
  int tc = mc - 6;
  switch(c){
    case GLFW_GAMEPAD_AXIS_LEFT_X: return "Stick-L-X+";
    case GLFW_GAMEPAD_AXIS_LEFT_Y: return "Stick-L-Y+";
    case GLFW_GAMEPAD_AXIS_RIGHT_X: return "Stick-R-X+";
    case GLFW_GAMEPAD_AXIS_RIGHT_Y: return "Stick-R-Y+";
    case GLFW_GAMEPAD_AXIS_LEFT_TRIGGER: return "Trigger-L+";
    case GLFW_GAMEPAD_AXIS_RIGHT_TRIGGER: return "Trigger-R+";
  }
  switch(mc){
    case GLFW_GAMEPAD_AXIS_LEFT_X: return "Stick-L-X-";
    case GLFW_GAMEPAD_AXIS_LEFT_Y: return "Stick-L-Y-";
    case GLFW_GAMEPAD_AXIS_RIGHT_X: return "Stick-R-X-";
    case GLFW_GAMEPAD_AXIS_RIGHT_Y: return "Stick-R-Y-";
    case GLFW_GAMEPAD_AXIS_LEFT_TRIGGER: return "Trigger-L-";
    case GLFW_GAMEPAD_AXIS_RIGHT_TRIGGER: return "Trigger-R-";
  }
  switch(tc){
    case GLFW_GAMEPAD_BUTTON_A: return "A";
    case GLFW_GAMEPAD_BUTTON_B: return "B";
    case GLFW_GAMEPAD_BUTTON_X: return "X";
    case GLFW_GAMEPAD_BUTTON_Y: return "Y";
    case GLFW_GAMEPAD_BUTTON_LEFT_BUMPER: return "Bumper-L";
    case GLFW_GAMEPAD_BUTTON_RIGHT_BUMPER: return "Bumper-R";
    case GLFW_GAMEPAD_BUTTON_BACK: return locale.getTranslation(GamepadKeyBack);
    case GLFW_GAMEPAD_BUTTON_START: return "Start";
    case GLFW_GAMEPAD_BUTTON_GUIDE: return "Guide";
    case GLFW_GAMEPAD_BUTTON_LEFT_THUMB: return "Stick-L";
    case GLFW_GAMEPAD_BUTTON_RIGHT_THUMB: return "Stick-R";
    case GLFW_GAMEPAD_BUTTON_DPAD_UP: return locale.getTranslation(DpadUp);
    case GLFW_GAMEPAD_BUTTON_DPAD_RIGHT: return locale.getTranslation(DpadRight);
    case GLFW_GAMEPAD_BUTTON_DPAD_DOWN: return locale.getTranslation(DpadDown);
    case GLFW_GAMEPAD_BUTTON_DPAD_LEFT: return locale.getTranslation(DpadLeft);
  }
  return "?";
}

std::string translateKeyCode(int c){
  switch(c){
    case GLFW_KEY_SPACE: return locale.getTranslation(KeySpace);
    case GLFW_KEY_APOSTROPHE: return "'";
    case GLFW_KEY_COMMA: return ",";
    case GLFW_KEY_MINUS: return "-";
    case GLFW_KEY_PERIOD: return ".";
    case GLFW_KEY_SLASH: return "/";
    case GLFW_KEY_SEMICOLON: return ";";
    case GLFW_KEY_EQUAL: return "=";
    case GLFW_KEY_LEFT_BRACKET: return "[";
    case GLFW_KEY_BACKSLASH: return "\\";
    case GLFW_KEY_RIGHT_BRACKET: return "]";
    case GLFW_KEY_GRAVE_ACCENT: return "Grave";
    case GLFW_KEY_WORLD_1: return "World1";
    case GLFW_KEY_WORLD_2: return "World2";
    case GLFW_KEY_ESCAPE: return locale.getTranslation(KeyEsc);
    case GLFW_KEY_ENTER: return locale.getTranslation(KeyEnter);
    case GLFW_KEY_TAB: return locale.getTranslation(KeyTab);
    case GLFW_KEY_BACKSPACE: return locale.getTranslation(KeyBackspace);
    case GLFW_KEY_INSERT: return locale.getTranslation(KeyInsert);
    case GLFW_KEY_DELETE: return locale.getTranslation(KeyDelete);
    case GLFW_KEY_RIGHT: return locale.getTranslation(KeyRight);
    case GLFW_KEY_LEFT: return locale.getTranslation(KeyLeft);
    case GLFW_KEY_DOWN: return locale.getTranslation(KeyDown);
    case GLFW_KEY_UP: return locale.getTranslation(KeyUp);
    case GLFW_KEY_PAGE_UP: return locale.getTranslation(KeyPageUp);
    case GLFW_KEY_PAGE_DOWN: return locale.getTranslation(KeyPageDown);
    case GLFW_KEY_HOME: return locale.getTranslation(KeyHome);
    case GLFW_KEY_END: return locale.getTranslation(KeyEnd);
    case GLFW_KEY_CAPS_LOCK: return locale.getTranslation(KeyCapsLock);
    case GLFW_KEY_SCROLL_LOCK: return locale.getTranslation(KeyScrollLock);
    case GLFW_KEY_NUM_LOCK: return locale.getTranslation(KeyNumLock);
    case GLFW_KEY_PRINT_SCREEN: return locale.getTranslation(KeyPrint);
    case GLFW_KEY_PAUSE: return locale.getTranslation(KeyPause);
    case GLFW_KEY_F1: return locale.getTranslation(KeyF1);
    case GLFW_KEY_F2: return locale.getTranslation(KeyF2);
    case GLFW_KEY_F3: return "F3";
    case GLFW_KEY_F4: return "F4";
    case GLFW_KEY_F5: return "F5";
    case GLFW_KEY_F6: return "F6";
    case GLFW_KEY_F7: return "F7";
    case GLFW_KEY_F8: return "F8";
    case GLFW_KEY_F9: return "F9";
    case GLFW_KEY_F10: return "F10";
    case GLFW_KEY_F11: return locale.getTranslation(KeyF11);
    case GLFW_KEY_F12: return "F12";
    case GLFW_KEY_F13: return "F13";
    case GLFW_KEY_F14: return "F14";
    case GLFW_KEY_F15: return "F15";
    case GLFW_KEY_F16: return "F16";
    case GLFW_KEY_F17: return "F17";
    case GLFW_KEY_F18: return "F18";
    case GLFW_KEY_F19: return "F19";
    case GLFW_KEY_F20: return "F20";
    case GLFW_KEY_F21: return "F21";
    case GLFW_KEY_F22: return "F22";
    case GLFW_KEY_F23: return "F23";
    case GLFW_KEY_F24: return "F24";
    case GLFW_KEY_F25: return "F25";
    case GLFW_KEY_KP_0: return "Num 0";
    case GLFW_KEY_KP_1: return "Num 1";
    case GLFW_KEY_KP_2: return "Num 2";
    case GLFW_KEY_KP_3: return "Num 3";
    case GLFW_KEY_KP_4: return "Num 4";
    case GLFW_KEY_KP_5: return "Num 5";
    case GLFW_KEY_KP_6: return "Num 6";
    case GLFW_KEY_KP_7: return "Num 7";
    case GLFW_KEY_KP_8: return "Num 8";
    case GLFW_KEY_KP_9: return "Num 9";
    case GLFW_KEY_KP_DECIMAL: return "Num .";
    case GLFW_KEY_KP_DIVIDE: return "Num /";
    case GLFW_KEY_KP_MULTIPLY: return "Num *";
    case GLFW_KEY_KP_SUBTRACT: return "Num -";
    case GLFW_KEY_KP_ADD: return "Num +";
    case GLFW_KEY_KP_ENTER: return locale.getTranslation(KeyNumEnter);
    case GLFW_KEY_KP_EQUAL: return "Num =";
    case GLFW_KEY_LEFT_SHIFT: return locale.getTranslation(KeyLShift);
    case GLFW_KEY_LEFT_CONTROL: return locale.getTranslation(KeyLCRTL);
    case GLFW_KEY_LEFT_ALT: return locale.getTranslation(KeyLAlt);
    case GLFW_KEY_LEFT_SUPER: return locale.getTranslation(KeyLSuper);
    case GLFW_KEY_RIGHT_SHIFT: return locale.getTranslation(KeyRShift);
    case GLFW_KEY_RIGHT_CONTROL: return locale.getTranslation(KeyRCRTL);
    case GLFW_KEY_RIGHT_ALT: return locale.getTranslation(KeyRAlt);
    case GLFW_KEY_RIGHT_SUPER : return locale.getTranslation(KeyRSuper);
    case GLFW_KEY_MENU: return locale.getTranslation(KeyMenu);
  }
  return "?";
}

std::string getBindingString(int controller, Action a, bool secondary){
    auto v = &globalConfig.controller1;
    if(controller==2){
        v = &globalConfig.controller2;
    }
    if(secondary){
        // Falsch, andere Funktion für Gamepad
        const std::string gamepadCode = translateGamepadCode((*v)[(int)a].second);
        const char* name = gamepadCode.c_str();
        return (name != nullptr) ? std::string(name) : std::string(1, (char)(*v)[(int)a].second);
    }
    else{
        const char* name = glfwGetKeyName((*v)[(int)a].first, glfwGetKeyScancode((*v)[(int)a].first));
        return (name != nullptr) ? std::string(name) : translateKeyCode((*v)[(int)a].first);
    }
}

std::vector<std::pair<std::string, ASMtype>> detectJumps(const std::pair<std::vector<std::pair<std::string, ASMtype>>, std::vector<int>> &lines){
  std::vector<std::pair<std::string, ASMtype>> result;
  
  if(lines.first.size()==0) return result;

  auto prevLine = lines.first[0];
  auto prevLength = lines.second[0];
  for(size_t i = 1; i < lines.first.size(); i++){
    // console->adressBytes : Adressbusbreite in Bytes
    auto oldpc = stoll(prevLine.first.substr(1, 2 * console->addressBytes()), 0, 16);
    auto newpc = stoll(lines.first[i].first.substr(1, 2 * console->addressBytes()), 0, 16);
    if(oldpc + prevLength != newpc){
      result.push_back({prevLine.first, ASM_JUMP});
    }
    else{
      result.push_back(prevLine);
    }

    prevLine = lines.first[i];
    prevLength = lines.second[i];
  }

  // Nicht benötigt, da die erste aktuelle Zeile hinten dran hängt
  // result.push_back(prevLine);

  return result;
}

std::vector<std::pair<std::string, ASMtype>> splitLines(const std::string &lines, bool old){
  std::stringstream f(lines);
  std::string line;
  std::vector<std::pair<std::string, ASMtype>> linesV;
  int i = 0;
  while(std::getline(f, line)){
    ASMtype t = i==0 && !old ? ASM_CURRENT : ASM_REGULAR;
    linesV.push_back({line, t});
    i++;
  }
  return linesV;
}

void Gui::ASMLine(std::string l, float r, float g, float b){
  ImGui::PushStyleColor(ImGuiCol_TextLink, ImVec4(r, g, b, 1.0f));
  ImGui::PushID(runningID++);
  if(ImGui::TextLink(l.substr(0, 2 * console->addressBytes() + 1).c_str())){
    auto pc = std::stoll(l.substr(1,2 * console->addressBytes() + 1).c_str(), 0, 16);
    breakpoints = console->addBreakpoint(pc);
  }
  ImGui::PopID();
  ImGui::PopStyleColor();
  ImGui::SameLine();
  ImGui::TextColored(ImVec4(r, g, b, 1.0f), "%s", l.substr(2 * console->addressBytes() + 1, l.size()- (2 * console->addressBytes() + 1)).c_str());
}

void Gui::printASM(const std::vector<std::pair<std::string, ASMtype>> &v){
  for(const auto &e : v){
    switch(e.second){
      case ASM_JUMP:
        ASMLine(e.first, 1.0f, 0.0f, 0.0f);
        ImGui::SeparatorText(locale.getTranslation(DebuggerJump).c_str());
        break;
      case ASM_CURRENT:
        ASMLine(e.first, 1.0f, 1.0f, 0.0f);
        break;
      case ASM_REGULAR:
        ASMLine(e.first, 1.0f, 1.0f, 1.0f);
        break;
    }
  }
}

void Gui::assemblyRender()
{
  auto nASM = console->getCurrentDisassembly();
  auto oASM = console->getOldDisassembly();
  auto oldLines = splitLines(oASM.first, true);
  auto newLines = splitLines(nASM.first, false);
  assert(oASM.second.size() == oldLines.size());
  auto oldLinesWithCurrent = oldLines;
  auto oldLengthsWithCurrent = oASM.second;
  if(newLines.size()>0 && nASM.second.size() > 0){
    oldLinesWithCurrent.push_back(newLines[0]);
    oldLengthsWithCurrent.push_back(nASM.second[0]);
  }
  oldLines = detectJumps({oldLinesWithCurrent, oldLengthsWithCurrent});
  printASM(oldLines);
  printASM(newLines);
}

void Gui::drawRegisters()
{
  console->displayRegisters();
}

void Gui::drawMemoryReader()
{
  ImGui::Text("%s", locale.getTranslation(DebuggerAddress).c_str());
  ImGui::SameLine();
  ImGui::PushID(runningID++);
  ImGui::InputText("", memInputBuf, 255);
  ImGui::PopID();
  if(ImGui::Button(locale.getTranslation(DebuggerRead1Byte).c_str())){
    std::string s(memInputBuf);
    removeCharsFromString(s, "x$");
    lastReadLow = 0;
    lastReadHigh = 0;
    lastHighLow = 0;
    lastHighHigh = 0;
    if(s.size()>0){
      auto addr = std::stoll(s, 0, 16);
      lastReadLow = console->readCpuBus(addr);
    }
  }
  ImGui::SameLine();
  if(ImGui::Button(locale.getTranslation(DebuggerRead2Byte).c_str())){
    std::string s(memInputBuf);
    removeCharsFromString(s, "x$");
    lastReadLow = 0;
    lastReadHigh = 0;
    lastHighLow = 0;
    lastHighHigh = 0;
    if(s.size()>0){
      auto addr = std::stoll(s, 0, 16);
      lastReadLow = console->readCpuBus(addr);
      lastReadHigh = console->readCpuBus(addr+1);
    }
  }
  if(console->addressBytes() > 2){
    ImGui::SameLine();
    if(ImGui::Button(locale.getTranslation(DebuggerRead4Byte).c_str())){
      std::string s(memInputBuf);
      removeCharsFromString(s, "x$");
      lastReadLow = 0;
      lastReadHigh = 0;
      lastHighLow = 0;
      lastHighHigh = 0;
      if(s.size()>0){
        auto addr = std::stoll(s, 0, 16);
        lastReadLow = console->readCpuBus(addr);
        lastReadHigh = console->readCpuBus(addr+1);
        lastHighLow = console->readCpuBus(addr+2);
        lastHighHigh = console->readCpuBus(addr+3);
      }
    }
  }

  ImGui::Text("%s", (locale.getTranslation(DebuggerHex)+getHex0x(getLastRead(),console->addressBytes() * 2)).c_str());
  ImGui::SameLine();
  ImGui::Text("%s", (locale.getTranslation(DebuggerOpcode)+(console->getOpcodeName(getLastRead()))).c_str());
  if(console->addressBytes() <= 2){
    ImGui::Text("%s", (locale.getTranslation(DebuggerBin)+std::bitset<16>(getLastRead()).to_string()).c_str());
  }
  else{
    ImGui::Text("%s", (locale.getTranslation(DebuggerBin)+std::bitset<32>(getLastRead()).to_string()).c_str());
  }
}

void Gui::drawBreakpoints()
{
  if(ImGui::Button(locale.getTranslation(DebuggerAdd).c_str())){
    std::string s(bpInputBuf);
    removeCharsFromString(s, "x$");
    if(s.size()>0){
      auto addr = std::stoll(s, 0, 16);
      breakpoints = console->addBreakpoint(addr);
    }
  }
  ImGui::SameLine();
  ImGui::PushID(runningID++);
  ImGui::InputText("", bpInputBuf, 255);
  ImGui::PopID();


  if(ImGui::Button(locale.getTranslation(DebuggerAddOp).c_str())){
    std::string s(opInputBuf);
    if(s.size()>0){
      breakpointsOP = console->addBreakpointOP(s);
    }
  }
  ImGui::SameLine();
  ImGui::PushID(runningID++);
  ImGui::InputText("", opInputBuf, 255);
  ImGui::PopID();

  ImGui::PushItemWidth(0);
  if(ImGui::BeginListBox("")){
    for(const auto &bp : breakpointsOP){
      ImGui::PushID(runningID++);
      ImGui::Text("%s", (locale.getTranslation(DebuggerBreakOp)+bp).c_str());
      ImGui::SameLine();
      if(ImGui::Button("X")){
        breakpointsOP = console->removeBreakpointOP(bp);
      }
      ImGui::PopID();
    }
    for(const auto &bp : breakpoints){
      ImGui::PushID(runningID++);
      ImGui::Text("%s", (locale.getTranslation(DebuggerBreakAddr)+getHex0x(bp, 2 * console->addressBytes())).c_str());
      ImGui::SameLine();
      if(ImGui::Button("X")){
        breakpoints = console->removeBreakpoint(bp);
      }
      ImGui::PopID();
    }
    ImGui::EndListBox();
  }
}

void Gui::drawDebugger()
{
  state->halt = console->isHalted();
  ImGui::Begin(locale.getTranslation(DebuggerDebugger).c_str(), &state->showDebugger, ImGuiWindowFlags_NoCollapse);
    ImGui::BeginTable(locale.getTranslation(DebuggerDebugger).c_str(), 2, ImGuiTableFlags_BordersInnerV);
      ImGui::TableNextColumn();
        assemblyRender();
      ImGui::TableNextColumn();
        bool h = state->halt;
        if(ImGui::Checkbox(locale.getTranslation(DebuggerBreakButton).c_str(), &h)){
          toggleHalt();
        }
        ImGui::SameLine();
        if(ImGui::Button(locale.getTranslation(DebuggerStep).c_str())){
          if(state->halt){
            console->addClock();
          }
        }
        ImGui::SeparatorText(locale.getTranslation(DebuggerRegister).c_str());
        drawRegisters();
        ImGui::SeparatorText(locale.getTranslation(DebuggerRam).c_str());
        drawMemoryReader();
        ImGui::SeparatorText(locale.getTranslation(DebuggerBreakpoints).c_str());
        drawBreakpoints();
    ImGui::EndTable();
  ImGui::End();
}

void Gui::drawAbout()
{
  ImGui::Begin(locale.getTranslation(AboutTitle).c_str(), &state->showAbout, ImGuiWindowFlags_NoCollapse);
    ImGui::Text("Anton's Multi-System-Emulator (AMSEL)");
    ImGui::Separator();
    ImGui::Text("%s", locale.getTranslation(AboutContent).c_str());
    ImGui::Text("%s", locale.getTranslation(AboutSystems).c_str());
    #ifdef BUILD_CGB
    ImGui::Text("%s", locale.getTranslation(ConsoleDescriptionCGB).c_str());
    #endif
    #ifdef BUILD_NES
    ImGui::Text("%s", locale.getTranslation(ConsoleDescriptionNES).c_str());
    #endif
    #ifdef BUILD_GBA
    ImGui::Text("%s", locale.getTranslation(ConsoleDescriptionGBA).c_str());
    #endif
    ImGui::TextLinkOpenURL(locale.getTranslation(AboutSource).c_str(), "https://github.com/aschmidt46/amsel");
  ImGui::End();
}

void Gui::drawOutput()
{
  ImGui::Begin(locale.getTranslation(TestOutput).c_str(), &state->showOutput, ImGuiWindowFlags_NoCollapse);
    if(ImGui::Button(locale.getTranslation(TestOutputStartAddress).c_str())){
      std::string s(outInputBuf);
      removeCharsFromString(s, "x$");
      if(s.size()>0){
        auto addr = std::stoll(s, 0, 16);
        outputStartsAt = addr;
      }
    }
    ImGui::SameLine();
    ImGui::PushID(runningID++);
    ImGui::InputText("##", outInputBuf, 255);
    ImGui::PopID();
    ImGui::TextWrapped("%s", console->getText(outputStartsAt).c_str());
  ImGui::End();
}

void Gui::drawControlSettings()
{
    if(ImGui::BeginViewportSideBar(locale.getTranslation(Controls).c_str(), ImGui::GetMainViewport(), ImGuiDir_Down, ImGui::GetMainViewport()->Size.y / 2, ImGuiWindowFlags_None)){
    bool cs = true;
    if(ImGui::BeginTabBar("TabBarInput##")){
        if(ImGui::BeginTabItem(locale.getTranslation(Controller1).c_str(), &cs, ImGuiTabItemFlags_NoReorder | (ImGuiTabItemFlags_)ImGuiTabItemFlags_NoCloseButton)){
          drawControlSettingsPage(1);
          ImGui::EndTabItem();
        }
        #ifdef FEATURE_LOCAL_MULTIPLAYER
        if(ImGui::BeginTabItem(locale.getTranslation(Controller2).c_str(), &cs, ImGuiTabItemFlags_NoReorder | (ImGuiTabItemFlags_)ImGuiTabItemFlags_NoCloseButton)){
          drawControlSettingsPage(2);
          ImGui::EndTabItem();
        }
        #endif
      ImGui::EndTabBar();
    }
  ImGui::End();}
}

void Gui::drawControlSettingsPage(int controller)
{
  if(ImGui::BeginTable(("Controller"+std::to_string(controller)+"Table##").c_str(), 3, ImGuiTableFlags_BordersInner)){

    ImGui::TableSetupColumn(locale.getTranslation(SAction).c_str());
    ImGui::TableSetupColumn(locale.getTranslation(BindingKeyboard).c_str());
    ImGui::TableSetupColumn(locale.getTranslation(BindingGamepad).c_str());
    ImGui::TableHeadersRow();
    
    ImGui::TableNextColumn();

    ImGui::Text("%s", locale.getTranslation(BindingDpadUp).c_str());
    ImGui::Text("%s", locale.getTranslation(BindingDpadDown).c_str());
    ImGui::Text("%s", locale.getTranslation(BindingDpadLeft).c_str());
    ImGui::Text("%s", locale.getTranslation(BindingDpadRight).c_str());
    ImGui::Text("%s", locale.getTranslation(BindingAButton).c_str());
    ImGui::Text("%s", locale.getTranslation(BindingBButton).c_str());
    ImGui::Text("%s", locale.getTranslation(BindingStartButton).c_str());
    ImGui::Text("%s", locale.getTranslation(BindingSelectButton).c_str());
    ImGui::Text("%s", locale.getTranslation(BindingLButton).c_str());
    ImGui::Text("%s", locale.getTranslation(BindingRButton).c_str());

    ImGui::TableNextColumn();

    for(int i = 0; i < 10; i++){
      buttonChangePrompt(i, controller, false);
    }

    ImGui::TableNextColumn();

    for(int i = 0; i < 10; i++){
      buttonChangePrompt(i, controller, true);
    }

    ImGui::EndTable();
    ImGui::Text("%s", locale.getTranslation(GamepadColon).c_str());
    ImGui::SameLine();
    const char* name;
    int jid = 0;

    if(controller==1) jid = globalConfig.jidController1;
    else jid = globalConfig.jidController2;
    name = glfwGetGamepadName(jid);

    std::string numberedName = (name==nullptr) ? (std::to_string(jid)+": ?") : std::to_string(jid) +": "+name;

    ImGui::PushID(runningID++);
    if(ImGui::BeginCombo("", numberedName.c_str())){
      for(const auto &j : connectedJoysticks){
        const char* localName = glfwGetGamepadName(j);
        std::string numberedLocalName = (localName==nullptr) ? (std::to_string(j)+": ?") : std::to_string(j) +": "+localName;
        bool selected = j == jid;
        ImGui::PushID(runningID++);
        if(ImGui::Selectable(numberedLocalName.c_str(), &selected)){
          changeGamepadTo(controller, j);
        }
        ImGui::PopID();
      }
      ImGui::EndCombo();
    }
    ImGui::PopID();
  }
}

void Gui::drawSystemOptions(){
  ImGui::Begin(locale.getTranslation(SettingsSystemOptions).c_str(), &state->showSystemOptions, ImGuiWindowFlags_NoCollapse);
    if(ImGui::BeginTable("SystemOptionsTable##", 2, ImGuiTableFlags_BordersInner)){
      ImGui::TableNextColumn();
      for(size_t i = 0; i < systemOptions.size(); i++){
        std::string& consoleName = systemOptions[i].first;
        ImGui::PushID(runningID++);
        if(ImGui::Selectable(consoleName.c_str(), i == state->systemOptionsIndex)){
          state->systemOptionsIndex = i;
        }
        ImGui::PopID();
      }
      // ImGui::EndListBox();
      ImGui::TableNextColumn();
      auto visitor = overload{
        [&](RequiredFile& f){
          ImGui::Text("%s", f.name.c_str());
          ImGui::SameLine();
          if(ImGui::Button("...")){
            auto result = pfd::open_file(f.name, globalConfig.directory, {f.extensions, f.extensions}, pfd::opt::none);
            auto res = result.result();
            if(res.size()>0){
              std::replace(res[0].begin(), res[0].end(), '\\', '/');
              f.path = res[0];
              FileIO::getInstance().saveSystemSettings(systemOptions);
            }
          }
          ImGui::SameLine();
          ImGui::InputText("", f.path.data(), 1024);
        },
        [&](Toggle& t){
          if(ImGui::Checkbox(t.name.c_str(), &t.value)){
            FileIO::getInstance().saveSystemSettings(systemOptions);
          }
        }
      };
      for(auto &option : *systemOptions[state->systemOptionsIndex].second){
        std::visit(visitor, option);
      }
      ImGui::EndTable();
    }
  ImGui::End();
}

void Gui::buttonChangePrompt(int i, unsigned int controller, bool secondary)
{
  ImGui::PushID(runningID++);
  std::string bString = (state->waitOn.wait && state->waitOn.actionToSet == (Action)i && state->waitOn.controller == controller && state->waitOn.secondary == secondary)
    ? locale.getTranslation(PressButtonPrompt)
    : getBindingString(controller, (Action)i, secondary);
  if(ImGui::SmallButton(bString.c_str())){
    state->waitOn.wait = !state->waitOn.wait;
    state->waitOn.controller = controller;
    state->waitOn.actionToSet = (Action)i;
    state->waitOn.secondary = secondary;
  }
  ImGui::PopID();
}

Gui::Gui(SharedState *state)
{
  this->state = state;
  for(int i = 0; i < 255; i++){
      memInputBuf[i] = 0;
      bpInputBuf[i] = 0;
      opInputBuf[i] = 0;
      outInputBuf[i] = 0;
  }
}

void Gui::render()
{
    runningID = 0;
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    if(state->showDebugger){
      drawDebugger();
    }

    if(state->showAbout){
      drawAbout();
    }

    if(state->showOutput){
      drawOutput();
    }

    if(state->showInput){
      drawControlSettings();
    }

    if(state->showSystemOptions){
      drawSystemOptions();
    }

    if(state->show){

      ImGui::BeginMainMenuBar();

      if (ImGui::BeginMenu(locale.getTranslation(MenuFile).c_str()))
        {
            if(ImGui::MenuItem(locale.getTranslation(FileLoad).c_str())){
              auto result = openFile();
              if(result.has_value()){
                std::filesystem::path p(result.value());
                gameTitle = p.filename().string();
                changeTitle = true;
                createConsole(result.value().c_str());
                console->setHalt(this->state->halt);
                for(auto bp : breakpoints){
                  console->addBreakpoint(bp);
                }
                for(auto bp : breakpointsOP){
                  console->addBreakpointOP(bp);
                }
                screen->updateFramebufferSize(globalConfig.sizeX, globalConfig.sizeY);
              }
            }
            // if(ImGui::MenuItem("Auswerfen")){
            //   if(console->loaded)
            //     console->ejectNextClock = true;
            // }
            ImGui::EndMenu();
        }
        // if (ImGui::BeginMenu("System"))
        // {
        //     // if (ImGui::MenuItem("RESET", "Esc")) {
        //     //   console->resetNextClock = true;
        //     // }
        //     ImGui::EndMenu();
        // }
        if(ImGui::BeginMenu(locale.getTranslation(MenuSettings).c_str())){
          if(ImGui::MenuItemEx(locale.getTranslation(SettingsSystemOptions).c_str(), ICON_FA_UP_RIGHT_FROM_SQUARE, "", state->showSystemOptions)){
            state->showSystemOptions = !state->showSystemOptions;
          }
          if(ImGui::MenuItemEx(locale.getTranslation(SettingsControls).c_str(), ICON_FA_UP_RIGHT_FROM_SQUARE, "", state->showInput)){
            state->showInput = !state->showInput;
          }
          ImGui::MenuItem(locale.getTranslation(SettingsFullscreen).c_str(), "F11", &state->fullScreen);
          if(ImGui::MenuItem(locale.getTranslation(SettingsCRTShader).c_str(), "", &globalConfig.useCRTShader)){
            FileIO::getInstance().saveSettings(globalConfig);
          };
          ImGui::MenuItem(locale.getTranslation(SettingsSound).c_str(), "", &globalConfig.unmute);
          if(ImGui::BeginMenuEx(locale.getTranslation(SettingsLanguage).c_str(), ICON_FA_GLOBE)){
            for(const auto &lang : availableLocales){
              bool selected = locale.getName()== lang.getName();
              if(ImGui::MenuItem(lang.getName().c_str(), "", &selected)){
                locale = lang;
                globalConfig.language = locale.getCode();
                FileIO::getInstance().saveSettings(globalConfig);
              }
            }
            ImGui::EndMenu();
          }
          ImGui::EndMenu();
        }
        if(ImGui::BeginMenu(locale.getTranslation(MenuExtras).c_str())){
          #ifdef FEATURE_DEBUGGER
          if(ImGui::MenuItemEx(locale.getTranslation(DebuggerDBItem).c_str(), ICON_FA_UP_RIGHT_FROM_SQUARE, "F1", state->showDebugger)){
            toggleDebugger();
          }

          if(ImGui::MenuItemEx(locale.getTranslation(DebuggerText).c_str(), ICON_FA_UP_RIGHT_FROM_SQUARE, "F2", state->showOutput)){
            toggleTestRomOutput();
          }
          #endif
          if(ImGui::MenuItemEx(locale.getTranslation(AboutTitle).c_str(), ICON_FA_INFO, "", state->showAbout)){
            state->showAbout = !state->showAbout;
          }
          ImGui::EndMenu();
        }
        bool s = globalConfig.unmute;
        if(!s) ImGui::BeginDisabled();
          ImGui::Text("%s", getVolumeIcon(globalConfig.volume, globalConfig.unmute));
          ImGui::SetNextItemWidth(-10);
          if(ImGui::SliderFloat("", &globalConfig.volume, 0, 1, "%.2f"))
            FileIO::getInstance().saveSettings(globalConfig);
        if(!s) ImGui::EndDisabled();

      ImGui::EndMainMenuBar();
    }

    MessageStruct m;
    while(messageQueue.try_dequeue(m)){
      ImGuiToast toast(mTypeToImGui(m.type), 8000);
      if(m.title.has_value())
        toast.setTitle(m.title.value().c_str());
      toast.setContent(m.content.c_str());
      ImGui::InsertNotification(toast);
    }

    // Notifications
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.f);
        
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.10f, 0.10f, 0.10f, 1.00f)); // Background color
        
    ImGui::RenderNotifications();
    ImGui::PopStyleVar(2);
    ImGui::PopStyleColor(1);

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void Gui::toggleDebugger()
{
  if(!state->showDebugger){
    state->showDebugger = true;
    console->produceDisassembly(true);
  }
  else{
    state->showDebugger = false;
    console->produceDisassembly(false);
  }
}

void Gui::toggleTestRomOutput()
{
  state->showOutput = !state->showOutput;
}
