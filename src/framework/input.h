#pragma once

#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include <vector>

void initInput();

// Callbacks
void mouseCallback(GLFWwindow* window, int button, int action, int mods);
void gamepadCallback(int controller, int key, int action);
void pollGamepadEvents();
void joystickCallback(int jid, int event);
void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);

// Eingabelogik
std::pair<std::vector<int>, std::vector<int>> calculateGamepadStateDelta(const GLFWgamepadstate &prev, const GLFWgamepadstate &now);
std::vector<int> enumerateGamepads();
void changeGamepadTo(int number, int jid);
