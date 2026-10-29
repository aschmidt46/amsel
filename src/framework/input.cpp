#include "input.h"

#include "framework/file_io.h"
#include "framework/global.h"

#include "imgui_impl_glfw.h"

#include "gui.h"
#include "screen.h"
#include <algorithm>

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
  glfwSetCursorPosCallback(window, mousePosCallback);
};

void mousePosCallback(GLFWwindow* window, double xpos, double ypos)
{
  if(console->getTouchArea().has_value()){
    const auto rect = console->getTouchArea().value();
    const float cx = console->getX(); float cy = console->getY();
    const float screenWidth = screen->right - screen->left;
    const float screenHeight = screen->bottom - screen->top;

    const float touchWidth = rect.right - rect.left; // Größe von Touch Areal in Pixeln (In Konsolen-Koordinaten)
    const float touchHeight = rect.bottom - rect.top;

    const float xWidthFactor = (touchWidth) / cx; // Wie viel Vom Konsolenbildschirm ist von Touchg "bedeckt" (1 = gesamte Breite)
    const float yWidthFactor = (touchHeight) / cy;

    const float x0 = screen->left + (rect.left / cx) * screenWidth; // In Bildschirmkoordinaten
    const float x1 = screen->left + (rect.right / cx) * screenWidth;

    const float y0 = screen->top + (rect.top / cy) * screenHeight;
    const float y1 = screen->top + (rect.bottom / cy) * screenHeight;

    if(xpos >= x0 && xpos < x1 && ypos >= y0 && ypos < y1){
        glfwSetCursor(window, touchCursor);
        const float xw = (xpos - x0) / (xWidthFactor * screenWidth); // Positionen auf Konsolenbildschirm zwischen 0 und 1
        const float yw = (ypos - y0) / (yWidthFactor * screenHeight);
        if(mouseIsDown){
          // std::cout << "x: " << xw << ", y: " << yw << "\n";
          console->setTouch(xw * touchWidth, yw * touchHeight);
        }
    }
    else{
      glfwSetCursor(window, NULL);
    }
  }

  ImGui_ImplGlfw_CursorPosCallback(window, xpos, ypos);
}

void mouseCallback(GLFWwindow* window, int button, int action, int mods){
  mouseIsDown = action > 0;
  if(mouseIsDown){
    double x, y;
    glfwGetCursorPos(window, &x, &y);
    mousePosCallback(window, x, y);
  }
  else console->releaseTouch();

  ImGui_ImplGlfw_MouseButtonCallback(window, button, action, mods);

  if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS)
    sharedGui->state->show = !sharedGui->state->show;
}

void gamepadCallback(int controller, int key, int action){

  // Warten und zwar nicht auf Tastatur, sondern Gamepad
  if(sharedGui->state->waitOn.wait && sharedGui->state->waitOn.secondary){
    auto gameplayAction = sharedGui->state->waitOn.actionToSet;
    sharedGui->state->waitOn.console->controllers[sharedGui->state->waitOn.controller][gameplayAction].second = key;
    sharedGui->state->waitOn.wait = false;
    FileIO::getInstance().saveSystemControls(sharedGui->state->waitOn.consoleName, sharedGui->state->waitOn.console);
    // Diese Eingabe abfangen
    return;
  }


  {
    std::lock_guard lock{consoleLock};
    const auto amselAction = calculateAmselAction(console->getControlOptions(), key, true);
    if(amselAction.first != AmselControlAction_Invalid)
      console->setKey(amselAction.second, amselAction.first, action);
  }
}

void pollGamepadEvents(){
  GLFWgamepadstate gamepad1Now;
  glfwGetGamepadState(globalConfig.jidController1, &gamepad1Now);
  auto gamepad1Delta = calculateGamepadStateDelta(previousState1, gamepad1Now);

  GLFWgamepadstate gamepad2Now;
  glfwGetGamepadState(globalConfig.jidController2, &gamepad2Now);
  auto gamepad2Delta = calculateGamepadStateDelta(previousState2, gamepad2Now);

  for(int i = 0; i <= 1; i++){
    int key = -1;
    auto delta = i==0 ? gamepad1Delta : gamepad2Delta;
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

void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
  (void)window; (void)scancode; (void)mods;
  if(action != GLFW_PRESS && action != GLFW_RELEASE) return;
  unsigned int v = action;

  // Warten und zwar nicht auf Gamepad, sondern Tastatur
  if(sharedGui->state->waitOn.wait && !sharedGui->state->waitOn.secondary){
    auto gameplayAction = sharedGui->state->waitOn.actionToSet;
    sharedGui->state->waitOn.console->controllers[sharedGui->state->waitOn.controller][gameplayAction].first = key;
    sharedGui->state->waitOn.wait = false;
    FileIO::getInstance().saveSystemControls(sharedGui->state->waitOn.consoleName, sharedGui->state->waitOn.console);
    // Diese Eingabe abfangen
    return;
  }


  {
    std::lock_guard lock{consoleLock};
    // if(v==1 && key == GLFW_KEY_ESCAPE){
    //   console->reset();
    // }
    if(v==1 && key == GLFW_KEY_F11){
      sharedGui->state->fullScreen = !sharedGui->state->fullScreen;
    }
    if(v==1 && key == GLFW_KEY_F1){
      sharedGui->toggleDebugger();
    }
    if(v==1 && key == GLFW_KEY_F2){
      sharedGui->toggleTestRomOutput();
    }
    const auto amselAction = calculateAmselAction(console->getControlOptions(), key, false);
    if(amselAction.first != AmselControlAction_Invalid)
      console->setKey(amselAction.second, amselAction.first, action);
  }
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
