#include <thread>

#include "framework/global.h"
#include "framework/audiosystem.h"
#include "framework/windowing.h"
#include "framework/input.h"
#include "framework/screen.h"
#include "nes/nes.h"
#include "gui.h"

#ifdef NES_ON_WINDOWS
  #include <Windows.h>
#endif



// Globale Variablen
// ---------------------------
std::mutex framebufferM;
std::filesystem::path exeDir;
moodycamel::BlockingConcurrentQueue<MessageStruct> messageQueue;
SettingsConfig globalConfig;
std::vector<int> connectedJoysticks;
Gui* sharedGui;
Screen* screen;
NES* console;
GLFWwindow* window;
// ---------------------------



int run()
{
  window  = initWindow();
  initInput();
  screen = new Screen();
  console = new NES(screen);
  AudioSystem audiosystem(console);
  SharedState state;
  postInit();
  
#ifdef _DEBUG
  Gui gui(console, &state, true);
#else
  Gui gui(console, &state, false);
#endif

  sharedGui = &gui;

  // Audiosystem taktet die Konsole in separatem Thread
  std::thread t(&AudioSystem::start, &audiosystem);


  while(!glfwWindowShouldClose(window)){
    do {
      onWindowUpdate();
      screen->present();
      gui.render();
      glfwSwapBuffers(window);
      updateOtherViewports();

    } while (!console->frameReady && !glfwWindowShouldClose(window));

    {
      // std::lock_guard<std::mutex> lock(framebufferM);
      console->frameReady = false;
      screen->copyBufferToScreen(console->ppu->backBuffer);
    }
  }

  audiosystem.close = true;
  t.join();
  cleanUp(window);
  return 0;
}

// Einstiegspunkte
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
