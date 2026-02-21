#include "windowing.h"

#include <iostream>

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include "icon.h"
#include "styles.h"
#include "ImGuiNotify.hpp"
#include "IconsFontAwesome6.h"
#include "fa-solid-900.h"
#include "file_io.h"
#include "whereami.h"

#include "gui.h"
#include "input.h"
#include "framework/global.h"
#include "framework/screen.h"
#include "nes/nes.h"

int width = 256, height = 240;

std::string title = "Anton's NES-Emulator";

bool wasFullscreen = false;


GLFWwindow* initWindow(){
  int result = glfwInit();
  if (!result) { std::cout << ("glfw init failed!") << std::endl; return nullptr; }
  glfwWindowHint( GLFW_CONTEXT_VERSION_MAJOR, 4 );
  glfwWindowHint( GLFW_CONTEXT_VERSION_MINOR, 6 );
  glfwWindowHint( GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE );
  GLFWwindow* window = glfwCreateWindow(2*width, 2*height, title.c_str(), NULL, NULL);
  glfwMakeContextCurrent(window);
  glViewport(0, 0, width, height);
  glfwSwapInterval(1);
  
  GLenum res = glewInit();
  if (res != GLEW_OK) {
    fprintf(stderr, "Error: '%s'\n", glewGetErrorString(res));
  }
  
  glDisable(GL_MULTISAMPLE);
  glDisable(GL_DEPTH_TEST);
  glDisable(GL_CULL_FACE);

  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO& io = ImGui::GetIO();
  // io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
  // io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
  io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;       // Multi Viewport für Debugger (braucht docking branch)
  io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;         // IF using Docking Branch

  // Setup Platform/Renderer backends
  ImGui_ImplGlfw_InitForOpenGL(window, true);          // Second param install_callback=true will install GLFW callbacks and chain to existing ones.
  ImGui_ImplOpenGL3_Init();

  io.Fonts->AddFontDefault();

  float baseFontSize = 16.0f;
  float iconFontSize = baseFontSize * 2.0f / 3.0f; // FontAwesome fonts need to have their sizes reduced by 2.0f/3.0f in order to align correctly

  static constexpr ImWchar iconsRanges[] = {ICON_MIN_FA, ICON_MAX_16_FA, 0};
  ImFontConfig iconsConfig;
  iconsConfig.MergeMode = true;
  iconsConfig.PixelSnapH = true;
  iconsConfig.GlyphMinAdvanceX = iconFontSize;
  io.Fonts->AddFontFromMemoryCompressedTTF(fa_solid_900_compressed_data, fa_solid_900_compressed_size, iconFontSize, &iconsConfig, iconsRanges);

    // Nur im Falle, dass noch keine Ini existiert.
  int autoX, autoY;
  glfwGetWindowPos(window, &autoX, &autoY);

  // Config laden
  globalConfig = FileIO::getInstance().loadSettings(autoX, autoY);
  glfwSetWindowPos(window, globalConfig.posX, globalConfig.posY);
  glfwSetWindowSize(window, globalConfig.sizeX, globalConfig.sizeY);
  if(globalConfig.maximize)
    glfwMaximizeWindow(window);
  

  // Verzeichnis finden
  int pathLength = wai_getExecutablePath(NULL, 0, NULL);
  char* p = (char*)malloc(pathLength + 1);
  int dirNameLength = 0;
  wai_getExecutablePath(p, 4096, &dirNameLength);
  p[pathLength] = '\0';
  std::filesystem::path path(p);
  free(p);
  exeDir = path.parent_path();

  glfwSetWindowMaximizeCallback(window, maximizeCallback);
  glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);
  glfwSetWindowPosCallback(window, positionCallback);
  const GLFWimage glfwIcon = {200,200,icon};
  glfwSetWindowIcon(window, 1, &glfwIcon);

  float xscale, yscale;
  glfwGetWindowContentScale(window, &xscale, &yscale);
  float mScale = 1.15 * (xscale + yscale) / 2.0;

  SetupImGuiStyle();
  // Skalierung nach Auflösung
  ImGuiStyle& style = ImGui::GetStyle();
  style.FontScaleDpi = mScale;
  style.ScaleAllSizes(mScale);


  return window;
}

void postInit()
{
  int fw, fh;
  glfwGetFramebufferSize(window, &fw, &fh);
  screen->updateFramebufferSize(fw, fh);
}

void cleanUp(GLFWwindow* window){
  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  ImGui::DestroyContext();
  glfwDestroyWindow(window);
  delete screen;
  delete console;
}

static void framebufferSizeCallback(GLFWwindow* window, int w, int h){
    width = w;
    height = h;
    screen->updateFramebufferSize(w, h);

    // Nicht durch maximiertes Fenster und Vollbild beeinflussen lassen
    if(!sharedGui->state->fullScreen && !glfwGetWindowAttrib(window, GLFW_MAXIMIZED) && glfwGetWindowMonitor(window) == NULL){
      globalConfig.sizeX = w;
      globalConfig.sizeY = h;
      FileIO::getInstance().saveSettings(globalConfig);
    }
}



static void maximizeCallback(GLFWwindow* window, int maximized){
  globalConfig.maximize = maximized;
  FileIO::getInstance().saveSettings(globalConfig);
}

static void positionCallback(GLFWwindow* window, int posx, int posy){
  // Für Ini
  if(!sharedGui->state->fullScreen && !glfwGetWindowAttrib(window, GLFW_MAXIMIZED) && glfwGetWindowMonitor(window) == NULL){
    globalConfig.posX = posx;
    globalConfig.posY = posy;
    FileIO::getInstance().saveSettings(globalConfig);
  }
}

void onToggleFullscreen(GLFWwindow* window){
  if(!sharedGui->state->fullScreen){
      glfwSetWindowMonitor(window, NULL, globalConfig.posX, globalConfig.posY, globalConfig.sizeX, globalConfig.sizeY, 0);
      if(globalConfig.maximize)
        glfwMaximizeWindow(window);
    }
    else{
      GLFWmonitor* monitor = glfwGetPrimaryMonitor();
      const GLFWvidmode* mode = glfwGetVideoMode(monitor);
 
      glfwSetWindowMonitor(window, monitor, 0, 0, mode->width, mode->height, mode->refreshRate);
    }
}

void onWindowUpdate()
{
  glfwPollEvents();
  pollGamepadEvents();
  if(wasFullscreen != sharedGui->state->fullScreen){
    wasFullscreen = sharedGui->state->fullScreen;
    onToggleFullscreen(window);
  }
  if(console->changeTitle){
    updateTitle(window);
    console->changeTitle = false;
  }
}

void updateTitle(GLFWwindow* window){
  std::filesystem::path p(console->fileName);
    auto fn = title + " - " + p.filename().string();
    if(p.filename().string().size()==0){
      fn = title;
    }
    glfwSetWindowTitle(window, fn.c_str());
}

void updateOtherViewports()
{
  ImGuiIO& io = ImGui::GetIO();
  if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
  {
      ImGui::UpdatePlatformWindows();
      ImGui::RenderPlatformWindowsDefault();
      glfwMakeContextCurrent(window);
  }
}
