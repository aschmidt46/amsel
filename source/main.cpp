#include <iostream>
#include <string>
#include <fstream>

#include "lib.hpp"
#include "cputest.h"

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include "nes.h"
#include "controller.h"

int width = 256, height = 240;

Screen* screen;
Controller* controller1;
NES* console;

GLFWwindow* initGL(){
  int result = glfwInit();
  if (!result) { printf("glfw init failed!"); return nullptr; }
  glfwWindowHint( GLFW_CONTEXT_VERSION_MAJOR, 4 );
  glfwWindowHint( GLFW_CONTEXT_VERSION_MINOR, 6 );
  glfwWindowHint( GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE );
  GLFWwindow* window = glfwCreateWindow(width, height, "Anton's NES-Emulator", NULL, NULL);
  glfwMakeContextCurrent(window);
  glfwGetFramebufferSize(window, &width, &height);
  glViewport(0, 0, width, height);
  glfwSwapInterval(0);
  
  GLenum res = glewInit();
  if (res != GLEW_OK) {
    fprintf(stderr, "Error: '%s'\n", glewGetErrorString(res));
  }
  
  glDisable(GL_MULTISAMPLE);
  glDisable(GL_DEPTH_TEST);
  glDisable(GL_CULL_FACE);
  return window;
}

void cleanUp(GLFWwindow* window){
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
  controller1->setKey(key, v);
}

auto main() -> int
{
  //CPUTest test;
  GLFWwindow* window = initGL();
  screen = new Screen();
  controller1 = new Controller();
  console = new NES(screen, controller1);
  console->load("kungfu.nes");

  glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
  // input implementieren
  glfwSetKeyCallback(window, key_callback);
int i = 0;
  while(!glfwWindowShouldClose(window)){
    glfwPollEvents();
    glfwSwapBuffers(window);
    console->nextFrame();
  }

  cleanUp(window);
  return 0;
}
