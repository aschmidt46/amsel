#include "gui.h"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <string>
#include "portable-file-dialogs.h"
#include <filesystem>

std::optional<std::string> openFile(){
  auto result = pfd::open_file("Rom auswählen", std::filesystem::current_path().string(), { "*.nes" }, pfd::opt::none);
  auto res = result.result();
  if(res.size()==0){
    return {};
  }
  return res[0];
}

void Gui::render()
{
    if(show){
      ImGui_ImplOpenGL3_NewFrame();
      ImGui_ImplGlfw_NewFrame();
      ImGui::NewFrame();
      ImGui::BeginMainMenuBar();

      if (ImGui::BeginMenu("Datei"))
        {
            if(ImGui::MenuItem("Laden")){
              auto result = openFile();
              if(result.has_value()){
                console->load(result.value().c_str());
              }
            }
            if(ImGui::MenuItem("Auswerfen")){
              console->eject();
            }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("System"))
        {
            if (ImGui::MenuItem("RESET", "Esc")) {
              console->reset();
            }
            ImGui::EndMenu();
        }

      ImGui::EndMainMenuBar();
  
      ImGui::Render();
      ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    }
}