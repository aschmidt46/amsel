#include "gui.h"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <string>
#include "portable-file-dialogs.h"
#include <filesystem>
#include <vector>
#include <bitset>
#include "file_io.h"

#include "ImGuiNotify.hpp"
#include "IconsFontAwesome6.h"
#include "fa-solid-900.h"
#include <GLFW/glfw3.h>

std::string ghex(uintptr_t input){
    std::string str = std::format("{:x}", input);
    std::transform(str.begin(), str.end(), str.begin(), ::toupper);
    return str;
}

std::string ghexNorm(std::string s, int n){
    while(s.size() < n)
        s = "0"+s;
    return s;
}

void removeCharsFromString( std::string &str, const char* charsToRemove ) {
   for ( unsigned int i = 0; i < strlen(charsToRemove); ++i ) {
      str.erase( remove(str.begin(), str.end(), charsToRemove[i]), str.end() );
   }
}

std::optional<std::string> openFile(){
  auto result = pfd::open_file("Rom auswählen", std::filesystem::current_path().string() + "\\..\\roms", {"iNES Rom-Dateien (.nes)", "*.nes"}, pfd::opt::none);
  auto res = result.result();
  if(res.size()==0){
    return {};
  }
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

const char* translateGamepadCode(int c){
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
    case GLFW_GAMEPAD_BUTTON_BACK: return "Zurück";
    case GLFW_GAMEPAD_BUTTON_START: return "Start";
    case GLFW_GAMEPAD_BUTTON_GUIDE: return "Guide";
    case GLFW_GAMEPAD_BUTTON_LEFT_THUMB: return "Stick-L";
    case GLFW_GAMEPAD_BUTTON_RIGHT_THUMB: return "Stick-R";
    case GLFW_GAMEPAD_BUTTON_DPAD_UP: return "Hoch";
    case GLFW_GAMEPAD_BUTTON_DPAD_RIGHT: return "Rechts";
    case GLFW_GAMEPAD_BUTTON_DPAD_DOWN: return "Runter";
    case GLFW_GAMEPAD_BUTTON_DPAD_LEFT: return "Links";
  }
  return nullptr;
}

const char* translateKeyCode(int c){
  switch(c){
    case GLFW_KEY_SPACE: return "Space";
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
    case GLFW_KEY_ESCAPE: return "Esc (Achtung vorbelegt)";
    case GLFW_KEY_ENTER: return "Enter";
    case GLFW_KEY_TAB: return "Tab";
    case GLFW_KEY_BACKSPACE: return "Backspace";
    case GLFW_KEY_INSERT: return "Einfg";
    case GLFW_KEY_DELETE: return "Entf";
    case GLFW_KEY_RIGHT: return "Pfeil Rechts";
    case GLFW_KEY_LEFT: return "Pfeil Links";
    case GLFW_KEY_DOWN: return "Pfeil Runter";
    case GLFW_KEY_UP: return "Pfeil Hoch";
    case GLFW_KEY_PAGE_UP: return "Bild Hoch";
    case GLFW_KEY_PAGE_DOWN: return "Bild Runter";
    case GLFW_KEY_HOME: return "Pos 1";
    case GLFW_KEY_END: return "Ende";
    case GLFW_KEY_CAPS_LOCK: return "Feststell";
    case GLFW_KEY_SCROLL_LOCK: return "Rollen";
    case GLFW_KEY_NUM_LOCK: return "Num";
    case GLFW_KEY_PRINT_SCREEN: return "Druck";
    case GLFW_KEY_PAUSE: return "Pause";
    case GLFW_KEY_F1: return "F1 (Achtung vorbelegt)";
    case GLFW_KEY_F2: return "F2 (Achtung vorbelegt)";
    case GLFW_KEY_F3: return "F3";
    case GLFW_KEY_F4: return "F4";
    case GLFW_KEY_F5: return "F5";
    case GLFW_KEY_F6: return "F6";
    case GLFW_KEY_F7: return "F7";
    case GLFW_KEY_F8: return "F8";
    case GLFW_KEY_F9: return "F9";
    case GLFW_KEY_F10: return "F10";
    case GLFW_KEY_F11: return "F11 (Achtung vorbelegt)";
    case GLFW_KEY_F12: return "F12";
    case GLFW_KEY_F13: return "F13";
    case GLFW_KEY_F14: return "F14";
    case GLFW_KEY_F15: return "F15";
    case GLFW_KEY_F16: return "F16";
    case GLFW_KEY_F17: return "F17";
    case GLFW_KEY_F18: return "F18";
    case GLFW_KEY_F19: return "F19";
    case GLFW_KEY_F20: return "Space";
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
    case GLFW_KEY_KP_ENTER: return "Num Enter";
    case GLFW_KEY_KP_EQUAL: return "Num =";
    case GLFW_KEY_LEFT_SHIFT: return "Umschalt Links";
    case GLFW_KEY_LEFT_CONTROL: return "Strg Links";
    case GLFW_KEY_LEFT_ALT: return "Alt Links";
    case GLFW_KEY_LEFT_SUPER: return "Super Links";
    case GLFW_KEY_RIGHT_SHIFT: return "Shift Rechts";
    case GLFW_KEY_RIGHT_CONTROL: return "Strg Rechts";
    case GLFW_KEY_RIGHT_ALT: return "AltGr";
    case GLFW_KEY_RIGHT_SUPER : return "Super Rechts";
    case GLFW_KEY_MENU: return "Menu";
  }
  return "?";
}

// Noch separate Übersetzungsfunktion für andere Tasten bauen...
std::string getBindingString(int controller, Action a, bool secondary){
    auto v = &globalConfig.controller1;
    if(controller==2){
        v = &globalConfig.controller2;
    }
    if(secondary){
        // Falsch, andere Funktion für Gamepad
        const char* name = translateGamepadCode((*v)[(int)a].second);
        return (name != nullptr) ? std::string(name) : std::string(1, (char)(*v)[(int)a].second);
    }
    else{
        const char* name = glfwGetKeyName((*v)[(int)a].first, glfwGetKeyScancode((*v)[(int)a].first));
        return (name != nullptr) ? std::string(name) : std::string(translateKeyCode((*v)[(int)a].first));
    }
}

void changeGamepadTo(int number, int jid){
  if(number==1){
    if(globalConfig.jidController2 == jid){
      globalConfig.jidController2 = globalConfig.jidController1;
    }
    globalConfig.jidController1 = jid;
  }
  else{
    if(globalConfig.jidController1 == jid){
      globalConfig.jidController1 = globalConfig.jidController2;
    }
    globalConfig.jidController2 = jid;
  }
  FileIO::getInstance().saveSettings(globalConfig);
}


std::vector<std::pair<std::string, ASMtype>> detectJumps(const std::pair<std::vector<std::pair<std::string, ASMtype>>, std::vector<int>> &lines){
  std::vector<std::pair<std::string, ASMtype>> result;
  
  if(lines.first.size()==0) return result;

  auto prevLine = lines.first[0];
  auto prevLength = lines.second[0];
  for(int i = 1; i < lines.first.size(); i++){
    auto oldpc = stoi(prevLine.first.substr(1, 4), 0, 16);
    auto newpc = stoi(lines.first[i].first.substr(1, 4), 0, 16);
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

void Gui::ASMLine(std::string l, int id, float r, float g, float b){
  ImGui::PushStyleColor(ImGuiCol_TextLink, ImVec4(r, g, b, 1.0f));
  ImGui::PushID(runningID++);
  if(ImGui::TextLink(l.substr(0,5).c_str())){
    int pc = std::stoi(l.substr(1,5).c_str(), 0, 16);
    breakpoints = console->addBreakpoint(pc);
  }
  ImGui::PopID();
  ImGui::PopStyleColor();
  ImGui::SameLine();
  ImGui::TextColored(ImVec4(r, g, b, 1.0f), l.substr(5, l.size()-5).c_str());
}

void Gui::printASM(const std::vector<std::pair<std::string, ASMtype>> &v){
  int id = 0;
  for(const auto &e : v){
    id++;
    switch(e.second){
      case ASM_JUMP:
        ASMLine(e.first, id, 1.0f, 0.0f, 0.0f);
        ImGui::SeparatorText("Sprung");
        break;
      case ASM_CURRENT:
        ASMLine(e.first, id, 1.0f, 1.0f, 0.0f);
        break;
      case ASM_REGULAR:
        ASMLine(e.first, id, 1.0f, 1.0f, 1.0f);
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
  ImGui::BeginTable("Register", 2);
  ImGui::TableNextColumn();
    ImGui::Text(("P: "+std::bitset<8>(console->cpu->P).to_string()).c_str());
    ImGui::Text(("PC: $"+ghexNorm(ghex(console->cpu->PC), 4)).c_str());
    ImGui::Text(("SP: $"+ghexNorm(ghex(console->cpu->SP), 2)).c_str());
  ImGui::TableNextColumn();
    ImGui::Text(("A: $"+ghexNorm(ghex(console->cpu->A), 2)).c_str());
    ImGui::Text(("X: $"+ghexNorm(ghex(console->cpu->X), 2)).c_str());
    ImGui::Text(("Y: $"+ghexNorm(ghex(console->cpu->Y), 2)).c_str());
  ImGui::EndTable();
}

void Gui::drawMemoryReader()
{
  ImGui::Text("Adresse:");
  ImGui::SameLine();
  ImGui::PushID(runningID++);
  ImGui::InputText("", memInputBuf, 255);
  ImGui::PopID();
  if(ImGui::Button("Lies 1 Byte")){
    std::string s(memInputBuf);
    removeCharsFromString(s, "x$");
    lastReadLow = 0;
    lastReadHigh = 0;
    if(s.size()>0){
      uint16_t addr = std::stoi(s, 0, 16);
      lastReadLow = console->cpu->read((uint8_t*)(uintptr_t)addr);
    }
  }
  ImGui::SameLine();
  if(ImGui::Button("Lies 2 Byte")){
    std::string s(memInputBuf);
    removeCharsFromString(s, "x$");
    lastReadLow = 0;
    lastReadHigh = 0;
    if(s.size()>0){
      uint16_t addr = std::stoi(s, 0, 16);
      lastReadLow = console->cpu->read((uint8_t*)(uintptr_t)addr);
      lastReadHigh = console->cpu->read((uint8_t*)(uintptr_t)addr+1);
    }
  }

  ImGui::Text(("Hex: "+ghexNorm(ghex(getLastRead()),4)).c_str());
  ImGui::SameLine();
  ImGui::Text(("Opcode: "+(getLastRead() < 256 ? console->cpu->opcodes[getLastRead()].name : "???")).c_str());
  ImGui::Text(("Bin: "+std::bitset<16>(getLastRead()).to_string()).c_str());
}

void Gui::drawBreakpoints()
{
  if(ImGui::Button("Hinzufügen:")){
    std::string s(bpInputBuf);
    removeCharsFromString(s, "x$");
    if(s.size()>0){
      uint16_t addr = std::stoi(s, 0, 16);
      breakpoints = console->addBreakpoint(addr);
    }
  }
  ImGui::SameLine();
  ImGui::PushID(runningID++);
  ImGui::InputText("", bpInputBuf, 255);
  ImGui::PopID();


  if(ImGui::Button("Hinzufügen Op:")){
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
      ImGui::Text(("Break: "+bp).c_str());
      ImGui::SameLine();
      if(ImGui::Button("X")){
        breakpointsOP = console->removeBreakpointOP(bp);
      }
      ImGui::PopID();
    }
    for(const auto &bp : breakpoints){
      ImGui::PushID(runningID++);
      ImGui::Text(("Break: $"+ghexNorm(ghex(bp), 4)).c_str());
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
  state->halt = console->halt;
  ImGui::Begin("Debugger", &state->showDebugger, ImGuiWindowFlags_NoCollapse);
    ImGui::BeginTable("Debugger", 2, ImGuiTableFlags_BordersInnerV);
      ImGui::TableNextColumn();
        assemblyRender();
      ImGui::TableNextColumn();
        bool h = state->halt;
        if(ImGui::Checkbox("Break", &h)){
          toggleHalt();
        }
        ImGui::SameLine();
        if(ImGui::Button("Step")){
          if(state->halt){
            console->allowedClocks = 1;
          }
        }
        ImGui::SeparatorText("Register");
        drawRegisters();
        ImGui::SeparatorText("Speicher");
        drawMemoryReader();
        ImGui::SeparatorText("Breakpoints");
        drawBreakpoints();
    ImGui::EndTable();
  ImGui::End();
}

void Gui::drawOutput()
{
  ImGui::Begin("Test-ROM Ausgabe (falls vorhanden)", &state->showOutput, ImGuiWindowFlags_NoCollapse);
    if(ImGui::Button("Startadresse:")){
      std::string s(outInputBuf);
      removeCharsFromString(s, "x$");
      if(s.size()>0){
        uint16_t addr = std::stoi(s, 0, 16);
        outputStartsAt = addr;
      }
    }
    ImGui::SameLine();
    ImGui::PushID(runningID++);
    ImGui::InputText("##", outInputBuf, 255);
    ImGui::PopID();
    ImGui::TextWrapped(console->getText(outputStartsAt).c_str());
  ImGui::End();
}

void Gui::drawControlSettings()
{
    if(ImGui::BeginViewportSideBar("Steuerung", ImGui::GetMainViewport(), ImGuiDir_Down, ImGui::GetMainViewport()->Size.y / 2, ImGuiWindowFlags_None)){
    bool cs = true;
    if(ImGui::BeginTabBar("TabBarInput##")){
        if(ImGui::BeginTabItem("Controller 1", &cs, ImGuiTabItemFlags_NoReorder | ImGuiTabItemFlags_NoCloseButton)){
          drawControlSettingsPage(1);
          ImGui::EndTabItem();
        }
        if(ImGui::BeginTabItem("Controller 2", &cs, ImGuiTabItemFlags_NoReorder | ImGuiTabItemFlags_NoCloseButton)){
          drawControlSettingsPage(2);
          ImGui::EndTabItem();
        }
      ImGui::EndTabBar();
    }
  ImGui::End();}
}

void Gui::drawControlSettingsPage(int controller)
{
  if(ImGui::BeginTable(("Controller"+std::to_string(controller)+"Table##").c_str(), 3, ImGuiTableFlags_BordersInner)){

    ImGui::TableSetupColumn("Aktion");
    ImGui::TableSetupColumn("Belegung Tastatur");
    ImGui::TableSetupColumn("Belegung Gamepad");
    ImGui::TableHeadersRow();
    
    ImGui::TableNextColumn();

    ImGui::Text("Steuerkreuz Hoch");
    ImGui::Text("Steuerkreuz Runter");
    ImGui::Text("Steuerkreuz Links");
    ImGui::Text("Steuerkreuz Rechts");
    ImGui::Text("A-Taste");
    ImGui::Text("B-Taste");
    ImGui::Text("START-Taste");
    ImGui::Text("SELECT-Taste");

    ImGui::TableNextColumn();

    for(int i = 0; i < 8; i++){
      buttonChangePrompt(i, controller, false);
    }

    ImGui::TableNextColumn();

    for(int i = 0; i < 8; i++){
      buttonChangePrompt(i, controller, true);
    }

    ImGui::EndTable();
    ImGui::Text("Gamepad:");
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

void Gui::buttonChangePrompt(int i, int controller, bool secondary)
{
  ImGui::PushID(runningID++);
  std::string bString = (state->waitOn.wait && state->waitOn.actionToSet == (Action)i && state->waitOn.controller == controller && state->waitOn.secondary == secondary)
    ? "Drücke Taste..."
    : getBindingString(controller, (Action)i, secondary);
  if(ImGui::SmallButton(bString.c_str())){
    state->waitOn.wait = !state->waitOn.wait;
    state->waitOn.controller = controller;
    state->waitOn.actionToSet = (Action)i;
    state->waitOn.secondary = secondary;
  }
  ImGui::PopID();
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

    if(state->showOutput){
      drawOutput();
    }

    if(state->showInput){
      drawControlSettings();
    }

    if(state->show){

      ImGui::BeginMainMenuBar();

      if (ImGui::BeginMenu("Datei"))
        {
            if(ImGui::MenuItem("Laden")){
              auto result = openFile();
              if(result.has_value()){
                console->fileName = result.value();
                console->loadNextClock = true;
              }
            }
            if(ImGui::MenuItem("Auswerfen")){
              if(console->loaded)
                console->ejectNextClock = true;
            }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("System"))
        {
            if (ImGui::MenuItem("RESET", "Esc")) {
              console->resetNextClock = true;
            }
            ImGui::EndMenu();
        }
        if(ImGui::BeginMenu("Einstellungen")){
          if(ImGui::MenuItemEx("Steuerung", ICON_FA_UP_RIGHT_FROM_SQUARE, "", state->showInput)){
            state->showInput = !state->showInput;
          }
          ImGui::MenuItem("Vollbild", "F11", &state->fullScreen);
          ImGui::MenuItem("Ton", "", &console->sound);
          ImGui::EndMenu();
        }
        if(ImGui::BeginMenu("Debug")){
          if(ImGui::MenuItemEx("Debugger", ICON_FA_UP_RIGHT_FROM_SQUARE, "F1", state->showDebugger)){
            toggleDebugger();
          }

          if(ImGui::MenuItemEx("Textausgabe", ICON_FA_UP_RIGHT_FROM_SQUARE, "F2", state->showOutput)){
            toggleTestRomOutput();
          }
          ImGui::EndMenu();
        }
        bool s = console->sound;
        if(!s) ImGui::BeginDisabled();
          ImGui::Text(getVolumeIcon(globalConfig.volume, console->sound));
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
    console->produceDisassembly = true;
  }
  else{
    state->showDebugger = false;
    console->produceDisassembly = false;
  }
}

void Gui::toggleTestRomOutput()
{
  state->showOutput = !state->showOutput;
}
