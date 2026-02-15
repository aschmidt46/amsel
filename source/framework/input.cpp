#include "input.h"

#include "framework/file_io.h"
#include "framework/global.h"

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include "gui.h"
#include "nes/nes.h"

GLFWgamepadstate previousState1;
GLFWgamepadstate previousState2;

void initInput(){
  // input implementieren
  connectedJoysticks = enumerateGamepads();
  glfwGetGamepadState(globalConfig.jidController1, &previousState1);
  glfwGetGamepadState(globalConfig.jidController2, &previousState2);
  glfwSetJoystickCallback(joystickCallback);
  glfwSetKeyCallback(window, keyCallback);
  glfwSetMouseButtonCallback(window, mouseCallback);
};

static void mouseCallback(GLFWwindow* window, int button, int action, int mods){

  ImGui_ImplGlfw_MouseButtonCallback(window, button, action, mods);

  if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS)
    sharedGui->state->show = !sharedGui->state->show;
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
    console->setController1Key(true, key, action);
  }
  else{
    console->setController2Key(true, key, action);
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

static void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
  if(action != GLFW_PRESS && action != GLFW_RELEASE) return;
  unsigned int v = action;

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
  if(v==1 && key == GLFW_KEY_F1){
    sharedGui->toggleDebugger();
  }
  if(v==1 && key == GLFW_KEY_F2){
    sharedGui->toggleTestRomOutput();
  }
  console->setController1Key(false, key, action);
  console->setController2Key(false, key, action);
}

// -1 - Release, 0 - Nichts, 1 - Press
std::pair<std::vector<int>, std::vector<int>> calculateGamepadStateDelta(const GLFWgamepadstate &prev, const GLFWgamepadstate &now){
  std::pair<std::vector<int>, std::vector<int>> delta;
  delta.first = std::vector<int>(12);
  delta.second = std::vector<int>(15);
  const float axisThreshold = 0.75;

  // Delta für Positive Achsen
  for(int i = 0; i < 6; i++){
    if(prev.axes[i] > axisThreshold && now.axes[i] < axisThreshold){
      delta.first[i] = -1;
    }
    else if(prev.axes[i] < axisThreshold && now.axes[i] > axisThreshold){
      delta.first[i] = 1;
    }
    else delta.first[i] = 0;
  }
  // Delta für Negative Achsen
  for(int i = 6; i < 12; i++){
    if(prev.axes[i-6] < -axisThreshold && now.axes[i-6] > -axisThreshold){
      delta.first[i] = -1;
    }
    else if(prev.axes[i-6] > -axisThreshold && now.axes[i-6] < -axisThreshold){
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

std::vector<int> enumerateGamepads(){
  std::vector<int> pads;
  for(int i = 0; i < 10; i++){
    if(glfwJoystickPresent(i) && glfwJoystickIsGamepad(i)){
      pads.push_back(i);
    }
  }
  return pads;
}

void changeGamepadTo(int number, int jid){
  if(number==1){
    if(globalConfig.jidController2 == jid){
      globalConfig.jidController2 = globalConfig.jidController1;
    }
    globalConfig.jidController1 = jid;
  }
  else{
    if(globalConfig.jidController1 == jid){
      globalConfig.jidController1 = globalConfig.jidController2;
    }
    globalConfig.jidController2 = jid;
  }
  FileIO::getInstance().saveSettings(globalConfig);
}
