#pragma once
#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include <string>

GLFWwindow* initWindow();
void postInit();
void registerConsoles();
void cleanUp(GLFWwindow* window);

// Callbacks
void framebufferSizeCallback(GLFWwindow* window, int w, int h);
void maximizeCallback(GLFWwindow* window, int maximized);
void positionCallback(GLFWwindow* window, int posx, int posy);
void onToggleFullscreen(GLFWwindow* window);
void onWindowUpdate();

// Logik
void updateTitle(GLFWwindow* window);
void appendTitle(GLFWwindow* window, std::string title);
void updateOtherViewports();
