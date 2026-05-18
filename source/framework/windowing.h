#pragma once
#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include <string>

GLFWwindow* initWindow();
void postInit();
void cleanUp(GLFWwindow* window);

// Callbacks
static void framebufferSizeCallback(GLFWwindow* window, int w, int h);
static void maximizeCallback(GLFWwindow* window, int maximized);
static void positionCallback(GLFWwindow* window, int posx, int posy);
void onToggleFullscreen(GLFWwindow* window);
void onWindowUpdate();

// Logik
void updateTitle(GLFWwindow* window);
void updateOtherViewports();
