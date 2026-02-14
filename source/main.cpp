#include <iostream>
#include <string>
#include <fstream>

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include "nes.h"
#include "controller.h"

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "gui.h"


#include <chrono>
#include <thread>

#include <thread>

#include "audiosystem.h"
#include <filesystem>
#include <cstring>

#include "whereami.h"
#include "icon.h"
#include "styles.h"
#include "ImGuiNotify.hpp"
#include "IconsFontAwesome6.h"
#include "fa-solid-900.h"
#include "global.h"

#ifdef NES_ON_WINDOWS
  #include <Windows.h>
#endif
#include "file_io.h"


int width = 256, height = 240;

int oldwidth = width;
int oldheight = height;
int windowX, windowY;
int oldX = windowX;
int oldY = windowY;

Gui* sharedGui;

std::mutex cvm;

Screen* screen;
Controller* controller1;
Controller* controller2;
NES* console;

std::string title = "Anton's NES-Emulator";
std::filesystem::path exeDir;
bool wasFullscreen = false;
SettingsConfig globalConfig;
std::vector<int> connectedJoysticks;

moodycamel::BlockingConcurrentQueue<MessageStruct> messageQueue;

GLFWgamepadstate previousState1;
GLFWgamepadstate previousState2;

// -1 - Release, 0 - Nichts, 1 - Press
std::pair<std::vector<int>, std::vector<int>> calculateGamepadStateDelta(const GLFWgamepadstate &prev, const GLFWgamepadstate &now){
  std::pair<std::vector<int>, std::vector<int>> delta;
  delta.first = std::vector<int>(6);
  delta.second = std::vector<int>(15);

  for(int i = 0; i < 6; i++){
    if(prev.axes[i] > 0.5 && now.axes[i] < 0.5){
      delta.first[i] = -1;
    }
    if(prev.axes[i] > -0.5 && now.axes[i] < -0.5){
      delta.first[i] = -1;
    }
    if(prev.axes[i] < -0.5 && now.axes[i] > -0.5){
      delta.first[i] = 1;
    }
    else if(prev.axes[i] < 0.5 && now.axes[i] > 0.5){
      delta.first[i] = 1;
    }
    else delta.first[i] = 0;
  }

  for(int i = 0; i < 15; i++){
    if(prev.buttons[i] == 1 && now.buttons[i] == 0){
      delta.second[i] = -1;
    }
    else if(prev.buttons[i] == 0 && now.buttons[i] == 1){
      delta.second[i] = 1;
    }
    else delta.second[i] = 0;
  }

  return delta;
}


GLFWwindow* initGL(){
  int result = glfwInit();
  if (!result) { printf("glfw init failed!"); return nullptr; }
  glfwWindowHint( GLFW_CONTEXT_VERSION_MAJOR, 4 );
  glfwWindowHint( GLFW_CONTEXT_VERSION_MINOR, 6 );
  glfwWindowHint( GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE );
  GLFWwindow* window = glfwCreateWindow(2*width, 2*height, title.c_str(), NULL, NULL);
  glfwMakeContextCurrent(window);
  glfwGetFramebufferSize(window, &width, &height);
  glfwGetWindowPos(window, &windowX, &windowY);
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

  return window;
}

void cleanUp(GLFWwindow* window){
  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  ImGui::DestroyContext();
  glfwDestroyWindow(window);
  delete screen;
  delete controller1;
  delete controller2;
  delete console;
}

static void mouseCallback(GLFWwindow* window, int button, int action, int mods){

  ImGui_ImplGlfw_MouseButtonCallback(window, button, action, mods);

  if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS)
    sharedGui->state->show = !sharedGui->state->show;
}

static void framebufferSizeCallback(GLFWwindow* window, int w, int h){
    width = w;
    height = h;
    screen->updateFramebufferSize(w, h);
  };

void onToggleFullscreen(GLFWwindow* window){
  if(!sharedGui->state->fullScreen){
      GLFWmonitor* monitor = glfwGetPrimaryMonitor();
      glfwSetWindowMonitor(window, NULL, oldX, oldY, oldwidth, oldheight, 0);
      sharedGui->state->fullScreen = false;
    }
    else{
      GLFWmonitor* monitor = glfwGetPrimaryMonitor();
      const GLFWvidmode* mode = glfwGetVideoMode(monitor);
      oldwidth = width;
      oldheight = height;
      oldX = windowX;
      oldY = windowY;
 
      glfwSetWindowMonitor(window, monitor, 0, 0, mode->width, mode->height, mode->refreshRate);
      sharedGui->state->fullScreen = true;
    }
}

void gamepadCallback(int controller, int key, int action){

  // Warten und zwar nicht auf Tastatur, sondern Gamepad
  if(sharedGui->state->waitOn.wait && sharedGui->state->waitOn.secondary){
    auto conf = (sharedGui->state->waitOn.controller==1) ? &globalConfig.controller1 : &globalConfig.controller2;
    auto gameplayAction = sharedGui->state->waitOn.actionToSet;
    (*conf)[gameplayAction].second = key;
    sharedGui->state->waitOn.wait = false;
    FileIO::getInstance().saveSettings(globalConfig);
    // Diese Eingabe abfangen
    return;
  }


  if(controller == 1){
    controller1->setKey(true, key, action);
  }
  else{
    controller2->setKey(true, key, action);
  }
}

void pollGamepadEvents(){
  GLFWgamepadstate gamepad1Now;
  glfwGetGamepadState(globalConfig.jidController1, &gamepad1Now);
  auto gamepad1Delta = calculateGamepadStateDelta(previousState1, gamepad1Now);

  GLFWgamepadstate gamepad2Now;
  glfwGetGamepadState(globalConfig.jidController2, &gamepad2Now);
  auto gamepad2Delta = calculateGamepadStateDelta(previousState2, gamepad2Now);

  for(int i = 1; i <= 2; i++){
    int key = -1;
    auto delta = i==1 ? gamepad1Delta : gamepad2Delta;
    for(const auto &axis : delta.first){
      key++;
      if(axis==0) continue;
      // Umwandeln in [0,1] von [-1,1]
      gamepadCallback(i, key, (axis < 0) ? 0 : 1);
    }
    for(const auto &button : delta.second){
      key++;
      if(button==0) continue;
      gamepadCallback(i, key, (button < 0) ? 0 : 1);
    }
  }
  previousState1 = gamepad1Now;
  previousState2 = gamepad2Now;
}


void joystickCallback(int jid, int event){
  if(event == GLFW_CONNECTED){
    std::vector<int>::iterator it;
    it = std::find(connectedJoysticks.begin(), connectedJoysticks.end(), jid);
    // Joystick hinzufügen, wenn er nicht bereits existiert und ein Gamepad ist, Liste nach ID sortieren
    if (it == connectedJoysticks.end() && glfwJoystickIsGamepad(jid)) {
        connectedJoysticks.push_back(jid);
        std::sort(connectedJoysticks.begin(), connectedJoysticks.end());
    }
  }
  else if(event == GLFW_DISCONNECTED){
    connectedJoysticks.erase(std::remove_if(connectedJoysticks.begin(), connectedJoysticks.end(), 
                       [&](uint16_t i) { return i == jid; }), connectedJoysticks.end());
  }
}

std::vector<int> enumerateGamepads(){
  std::vector<int> pads;
  for(int i = 0; i < 10; i++){
    if(glfwJoystickPresent(i) && glfwJoystickIsGamepad(i)){
      pads.push_back(i);
    }
  }
  return pads;
}

static void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
  unsigned int v = 0;
  if(action == GLFW_PRESS) v = 1;
  if(action == GLFW_RELEASE) v = 0;
  if(action != GLFW_PRESS && action != GLFW_RELEASE) return;

  // Warten und zwar nicht auf Gamepad, sondern Tastatur
  if(sharedGui->state->waitOn.wait && !sharedGui->state->waitOn.secondary){
    auto conf = (sharedGui->state->waitOn.controller==1) ? &globalConfig.controller1 : &globalConfig.controller2;
    auto gameplayAction = sharedGui->state->waitOn.actionToSet;
    (*conf)[gameplayAction].first = key;
    sharedGui->state->waitOn.wait = false;
    FileIO::getInstance().saveSettings(globalConfig);
    // Diese Eingabe abfangen
    return;
  }


  if(v==1 && key == GLFW_KEY_ESCAPE){
    console->reset();
  }
  if(v==1 && key == GLFW_KEY_F11){
    sharedGui->state->fullScreen = !sharedGui->state->fullScreen;
  }
  controller1->setKey(false, key, v);
  controller2->setKey(false, key, v);
}

static void positionCallback(GLFWwindow* window, int posx, int posy){
  windowX = posx;
  windowY = posy;
}

void updateTitle(GLFWwindow* window){
  std::filesystem::path p(console->fileName);
    auto fn = title + " - " + p.filename().string();
    if(p.filename().string().size()==0){
      fn = title;
    }
    glfwSetWindowTitle(window, fn.c_str());
}

int run()
{
  GLFWwindow* window = initGL();
  screen = new Screen();
  controller1 = new Controller(false);
  controller2 = new Controller(true);
  console = new NES(screen, controller1, controller2);
  SharedState* state = new SharedState();
  AudioSystem audiosystem(console);

  globalConfig = FileIO::getInstance().loadSettings();
  
#ifdef _DEBUG
  Gui gui(console, state, true);
#else
  Gui gui(console, state, false);
#endif

  sharedGui = &gui;

  // Verzeichnis finden
  int pathLength = wai_getExecutablePath(NULL, 0, NULL);
  char* p = (char*)malloc(pathLength + 1);
  int dirNameLength = 0;
  wai_getExecutablePath(p, 4096, &dirNameLength);
  p[pathLength] = '\0';
  std::filesystem::path path(p);
  free(p);
  exeDir = path.parent_path();

  glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);
  // input implementieren
  connectedJoysticks = enumerateGamepads();
  glfwGetGamepadState(globalConfig.jidController1, &previousState1);
  glfwGetGamepadState(globalConfig.jidController2, &previousState2);
  glfwSetJoystickCallback(joystickCallback);
  glfwSetKeyCallback(window, keyCallback);
  glfwSetMouseButtonCallback(window, mouseCallback);
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




  std::thread t(&AudioSystem::start, &audiosystem);
  ImGuiIO& io = ImGui::GetIO();


  while(!glfwWindowShouldClose(window)){
    do {
      glfwPollEvents();
      pollGamepadEvents();
      if(wasFullscreen != sharedGui->state->fullScreen){
        wasFullscreen = sharedGui->state->fullScreen;
        onToggleFullscreen(window);
      }
      screen->present();
      gui.render();
      glfwSwapBuffers(window);
      
      // Zusätzliche Viewports
      if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
      {
          ImGui::UpdatePlatformWindows();
          ImGui::RenderPlatformWindowsDefault();
          glfwMakeContextCurrent(window);
      }

      if(console->changeTitle){
        updateTitle(window);
        console->changeTitle = false;
      }

    } while (!console->frameReady && !glfwWindowShouldClose(window));



    {
      std::lock_guard<std::mutex> lock(cvm);
      console->frameReady = false;
      screen->copyBufferToScreen(console->ppu->backBuffer);
    }
  }

  audiosystem.close = true;
  t.join();
  cleanUp(window);
  return 0;
}

#ifdef NES_ON_WINDOWS

  int APIENTRY WinMain(HINSTANCE hInst, HINSTANCE hInstPrev, PSTR cmdline, int cmdshow)
  {
    return run();
  }

#else

  int main()
  {
    return run();
  }

#endif
