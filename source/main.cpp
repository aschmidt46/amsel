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


int width = 256, height = 240;

int oldwidth = width;
int oldheight = height;
int windowX, windowY;
int oldX = windowX;
int oldY = windowY;

bool showGui;
bool fullScreen;

std::mutex cvm;

Screen* screen;
Controller* controller1;
NES* console;

GLFWwindow* initGL(){
  int result = glfwInit();
  if (!result) { printf("glfw init failed!"); return nullptr; }
  glfwWindowHint( GLFW_CONTEXT_VERSION_MAJOR, 4 );
  glfwWindowHint( GLFW_CONTEXT_VERSION_MINOR, 6 );
  glfwWindowHint( GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE );
  GLFWwindow* window = glfwCreateWindow(2*width, 2*height, "Anton's NES-Emulator", NULL, NULL);
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
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
  //io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;         // IF using Docking Branch

  // Setup Platform/Renderer backends
  ImGui_ImplGlfw_InitForOpenGL(window, true);          // Second param install_callback=true will install GLFW callbacks and chain to existing ones.
  ImGui_ImplOpenGL3_Init();

  return window;
}

void cleanUp(GLFWwindow* window){
  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  ImGui::DestroyContext();
  glfwDestroyWindow(window);
  delete screen;
  delete controller1;
  delete console;
}

static void framebuffer_size_callback(GLFWwindow* window, int w, int h){
    width = w;
    height = h;
    screen->updateFramebufferSize(w, h);
  };

static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
  unsigned int v = 0;
  if(action == GLFW_PRESS) v = 1;
  if(action == GLFW_RELEASE) v = 0;
  if(action != GLFW_PRESS && action != GLFW_RELEASE) return;
  if(v==1 && key == GLFW_KEY_ESCAPE){
    console->reset();
  }
  if(v==1 && key == GLFW_KEY_F11){
    if(fullScreen){
      GLFWmonitor* monitor = glfwGetPrimaryMonitor();
      glfwSetWindowMonitor(window, NULL, oldX, oldY, oldwidth, oldheight, 0);
      fullScreen = false;
    }
    else{
      GLFWmonitor* monitor = glfwGetPrimaryMonitor();
      const GLFWvidmode* mode = glfwGetVideoMode(monitor);
      oldwidth = width;
      oldheight = height;
      oldX = windowX;
      oldY = windowY;
 
      glfwSetWindowMonitor(window, monitor, 0, 0, mode->width, mode->height, mode->refreshRate);
      fullScreen = true;
    }
  }
  controller1->setKey(key, v);
}

static void position_callback(GLFWwindow* window, int posx, int posy){
  windowX = posx;
  windowY = posy;
}

auto main() -> int
{
  GLFWwindow* window = initGL();
  screen = new Screen();
  controller1 = new Controller();
  console = new NES(screen, controller1);
  AudioSystem audiosystem(console);
  Gui gui(console);

  glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
  // input implementieren
  glfwSetKeyCallback(window, key_callback);
  glfwSetWindowPosCallback(window, position_callback);

  float xscale, yscale;
  glfwGetWindowContentScale(window, &xscale, &yscale);
  float mScale = (xscale + yscale) / 2.0;

  // Skalierung nach Auflösung
  ImGuiStyle& style = ImGui::GetStyle();
  style.FontScaleDpi = mScale;
  style.ScaleAllSizes(mScale);



  std::thread t(&AudioSystem::start, &audiosystem);


  while(!glfwWindowShouldClose(window)){
    glfwPollEvents();
    while(!console->frameReady) {}

    {
      std::lock_guard<std::mutex> lock(cvm);
      console->frameReady = false;
      screen->copyBufferToScreen(console->ppu->backBuffer);
      screen->present();
      gui.render();
      glfwSwapBuffers(window);
    }

  }

  cleanUp(window);
  audiosystem.close = true;
  t.join();
  return 0;
}
