#pragma once
#include <glad/gl.h>
#include <GLFW/glfw3.h>

GLFWwindow* initWindow();
void postInit();
void cleanUp(GLFWwindow* window);

// Callbacks
void framebufferSizeCallback(GLFWwindow* window, int w, int h);
void maximizeCallback(GLFWwindow* window, int maximized);
void positionCallback(GLFWwindow* window, int posx, int posy);
void onToggleFullscreen(GLFWwindow* window);
void onWindowUpdate();

// Logik
void updateTitle(GLFWwindow* window);
void updateOtherViewports();
