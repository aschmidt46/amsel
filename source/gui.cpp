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

void Gui::render()
{
    if(state->show){
      ImGui_ImplOpenGL3_NewFrame();
      ImGui_ImplGlfw_NewFrame();
      ImGui::NewFrame();

      if(state->showDebugger){
        ImGui::Begin("Disassembly");
          ImGui::Text(console->getCurrentDisassembly().c_str());
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