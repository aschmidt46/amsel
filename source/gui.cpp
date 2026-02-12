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
  ImGui::Begin("Debugger", 0, ImGuiWindowFlags_NoCollapse);
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
  ImGui::Begin("Test-ROM Ausgabe (falls vorhanden)");
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
          if(ImGui::MenuItem("Vollbild", "F11")){
            state->fullScreen = !state->fullScreen;
          }
          bool ton = console->sound;
          if(ImGui::Checkbox("Ton", &ton)){
            console->sound = !console->sound;
          }
          float volume = console->volume;
          if(ImGui::SliderFloat("Lautstärke", &volume, 0, 1)){
            console->volume = volume;
          }
          ImGui::EndMenu();
        }
        if(ImGui::BeginMenu("Debug")){
          bool s = state->showDebugger;
          if(ImGui::Checkbox("Disassembler", &s)){
            toggleDebugger();
          }

          s = state->showOutput;
          if(ImGui::Checkbox("Textausgabe", &s)){
            toggleTestRomOutput();
          }
          ImGui::EndMenu();
        }

      ImGui::EndMainMenuBar();
    }

    if(console->unimplementedMapper>=0){
      int m = console->unimplementedMapper;
      console->unimplementedMapper = -1;
      ImGuiToast toast(ImGuiToastType::Error, 3000);
      toast.setTitle("Error");
      toast.setContent(("Dieses Spiel (iNES-Mapper "+std::to_string(m)+") wird nicht unterstützt, weil der entsprechende Mapper nicht implementiert ist.").c_str());
      ImGui::InsertNotification(toast);
    }

    if(FileIO::getInstance().shouldDisplayMessage()){
      ImGuiToast toast(ImGuiToastType::Success, 3000);
      toast.setTitle(std::filesystem::path(console->fileName).stem().string().c_str());
      toast.setContent("Ein Speicherstand wurde erstellt!");
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
