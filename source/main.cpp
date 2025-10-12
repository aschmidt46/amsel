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

 void APIENTRY glDebugProc(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length,
    const GLchar* message, const void* userParam){
        std::cout << "source: " << source <<std::endl;
        std::cout << "type: " << type <<std::endl;
        std::cout << "id: " << id <<std::endl;
        std::cout << "severity: " << severity <<std::endl;
        std::cout << "message: " << message <<std::endl;
        throw "GL ERR";
    }

GLFWwindow* initGL(){
  int result = glfwInit();
  if (!result) { printf("glfw init failed!"); return nullptr; }
  glfwWindowHint( GLFW_CONTEXT_VERSION_MAJOR, 4 );
  glfwWindowHint( GLFW_CONTEXT_VERSION_MINOR, 6 );
  glfwWindowHint( GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE );
  glfwWindowHint( GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE );
  glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GL_TRUE);
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
  //debug
  glEnable(GL_DEBUG_OUTPUT);
  glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
  glDebugMessageCallback(glDebugProc, nullptr);
  return window;
}

void cleanUp(GLFWwindow* window){
  glfwDestroyWindow(window);
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
  controller1->setKey(key, v);
}

auto main() -> int
{
  //CPUTest test;
  GLFWwindow* window = initGL();
  screen = new Screen();
  controller1 = new Controller();
  NES console(screen, controller1);
  console.load("nestest.nes");

  glfwSetWindowUserPointer(window, &screen);
  glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
  // input implementieren
  glfwSetKeyCallback(window, key_callback);

  while(!glfwWindowShouldClose(window)){
    glfwPollEvents();
    glfwSwapBuffers(window);
    int err = glGetError();
    while(err!=GL_NO_ERROR){
      std::cout << "OPENGL ERROR " << err << std::endl;
      err = glGetError();
    }
    console.nextFrame();
  }

  cleanUp(window);
  return 0;
}
