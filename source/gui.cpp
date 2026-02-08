#include "gui.h"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <string>
#include "portable-file-dialogs.h"
#include <filesystem>
#include <vector>

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
  int index = -1;
  ASMtype lastLineType = ASM_JUMP;
  for(int i = 0; i < oASM.size(); i++){
    index++;
    if(fastCompare(oASM[i].first, nASM[0].first)){
      index--;
      lastLineType = ASM_REGULAR;
      break;
    }
  }

  std::vector<std::pair<std::string, ASMtype>> resLines;

  for(int i = index-10; i <= index; i++){
    if(i < 0)
      continue;
    if(i==index) resLines.push_back({oASM[i].first, lastLineType});
    else resLines.push_back({oASM[i].first, ASM_REGULAR});
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

void Gui::render()
{
    if(state->show){
      ImGui_ImplOpenGL3_NewFrame();
      ImGui_ImplGlfw_NewFrame();
      ImGui::NewFrame();

      if(state->showDebugger){
        ImGui::Begin("Disassembly");
          ImGui::BeginTable("Debugger", 2, ImGuiTableFlags_BordersInnerH);
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
          ImGui::EndTable();
        ImGui::End();
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
