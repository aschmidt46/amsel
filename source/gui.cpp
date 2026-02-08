#include "gui.h"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <string>
#include "portable-file-dialogs.h"
#include <filesystem>
#include <vector>
#include <bitset>

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

std::vector<std::pair<std::string, ASMtype>> splitLines(const std::string &lines){
  std::stringstream f(lines);
  std::string line;
  std::vector<std::pair<std::string, ASMtype>> linesV;
  int i = 0;
  while(std::getline(f, line)){
    ASMtype t = i==0 ? ASM_CURRENT : ASM_REGULAR;
    linesV.push_back({line, t});
    i++;
  }
  return linesV;
}

// Program Counter
bool fastCompare(const std::string &s1, const std::string &s2){
  if(s1.size()<5 || s2.size()<5) return false;
  bool equal = true;
  for(int i = 0; i < 5; i++){
    if(s1[i]!=s2[i])
      equal = false;
  }
  return equal;
}

bool fastCompare(const std::vector<std::pair<std::string, ASMtype>> &v1, const std::vector<std::pair<std::string, ASMtype>> &v2){
  if(v1.size()==v2.size()){
    for(int i = 0; i < v1.size(); i++){
      if(!fastCompare(v1[i].first, v2[i].first))
        return false;
    }
  }
  return false;
}

bool isContained(const std::vector<std::pair<std::string, ASMtype>> &v1, const std::vector<std::pair<std::string, ASMtype>> &v2){
  // v1 in v2
  if(v1.size()==0) return false; //?

  if(v2.size()>= v1.size()){
    for(int i = 0; i < v2.size(); i++){
      if(fastCompare(v1[0].first, v2[i].first)){
        if(v2.size()-i > v1.size()) return false;
        if(i + v1.size() > v2.size()) return false;
        bool success = true;
        for(int j = 0; j < v1.size(); j++){
          if(!fastCompare(v2[i+j].first, v1[j].first)){
            success = false;
            break;
          }
        }
        if(success) return true;
      }
    }
  }
  return false;
}

std::vector<std::pair<std::string, ASMtype>> getPreceding(const std::vector<std::pair<std::string, ASMtype>> &nASM, const std::vector<std::pair<std::string, ASMtype>> &oASM){

  if(nASM.size()==0) return std::vector<std::pair<std::string, ASMtype>>();

  //Annahme: Sprung seit letztem Frame
  ASMtype lastLineType = ASM_JUMP;
  int index = -1;
  for(int i = 0; i < oASM.size(); i++){
    index++;
    if(fastCompare(oASM[i].first, nASM[0].first)){
      // Startzeile in altem ASM an Index gefunden (kein Sprung?)
      index--;
      // Letzte Zeile ist die vor Index
      lastLineType = ASM_REGULAR;
      break;
    }
  }

  // Korrektur für Sprungzeile
  if(lastLineType == ASM_JUMP){
    for(int i = 0; i < oASM.size(); i++){
      if(oASM[i].second == ASM_CURRENT){
        index = i;
        break;
      }
    }
  }

  std::vector<std::pair<std::string, ASMtype>> resLines;

  for(int i = index-10; i <= index; i++){
    if(i < 0)
      continue;
    if(i==index) resLines.push_back({oASM[i].first, lastLineType});
    else{
      if(oASM[i].second == ASM_CURRENT)
        resLines.push_back({oASM[i].first, ASM_REGULAR});
      else
        resLines.push_back({oASM[i]});
    }
  }

  for(const auto &e : nASM){
    resLines.push_back(e);
  }

  return resLines;
}

void printASM(const std::vector<std::pair<std::string, ASMtype>> &v){
  for(const auto &e : v){
    switch(e.second){
      case ASM_JUMP:
        ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), e.first.c_str());
        ImGui::SeparatorText("Sprung");
        break;
      case ASM_CURRENT:
        ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), e.first.c_str());
        break;
      case ASM_REGULAR:
        ImGui::Text(e.first.c_str());
        break;
    }
  }
}

void Gui::assemblyRender()
{
  std::string nASM = console->getCurrentDisassembly();
  auto lines = splitLines(nASM);
  auto prev = getPreceding(lines, oASM);
  bool wasIdentical = isContained(lines, oASM);
  if(wasIdentical)
    printASM(oASM);
  else{
    printASM(prev);
    oASM = prev;
  }
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
  ImGui::InputText("", memInputBuf, 255);
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
  ImGui::Text(("Bin: "+std::bitset<16>(getLastRead()).to_string()).c_str());
}

void Gui::drawDebugger()
{
  ImGui::Begin("Disassembly");
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
    ImGui::EndTable();
  ImGui::End();
}

void Gui::render()
{
    if(state->show){
      ImGui_ImplOpenGL3_NewFrame();
      ImGui_ImplGlfw_NewFrame();
      ImGui::NewFrame();

      if(state->showDebugger){
        drawDebugger();
      }

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
          ImGui::EndMenu();
        }

      ImGui::EndMainMenuBar();
  
      ImGui::Render();
      ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    }
}
