#include <thread>

#include "framework/global.h"
#include "framework/audiosystem.h"
#include "framework/windowing.h"
#include "framework/input.h"
#include "framework/screen.h"
#include "gui.h"
#include "console/dummy_implementation.h"
#include <cstring>

#ifdef NES_ON_WINDOWS
  #include <Windows.h>
#endif



// Globale Variablen
// ---------------------------
bool changeTitle = false;
std::string gameTitle = "";
Locale locale;
std::vector<Locale> availableLocales;
std::mutex framebufferM;
std::filesystem::path exeDir;
moodycamel::BlockingConcurrentQueue<MessageStruct> messageQueue;
SettingsConfig globalConfig;
std::vector<int> connectedJoysticks;
Gui* sharedGui;
Screen* screen;
Console* console;
std::mutex consoleLock;
GLFWwindow* window;
// ---------------------------



int run(int argc, wchar_t** argv)
{
  window  = initWindow();
  initInput();
  screen = new Screen();
  console = new DummyImplementation();
  AudioSystem audiosystem;
  SharedState state;
  postInit();

  Gui gui(&state);

  if(argc == 2){
    gui.state->show = false;
    gui.state->fullScreen = true;
    // Achtung, nur ASCII!
    std::wstring ws(argv[1]);
    std::string path(ws.begin(), ws.end());
    createConsole(path.c_str());
  }

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

    } while (!console->frameIsReady() && !glfwWindowShouldClose(window));

    {
      std::lock_guard lock{consoleLock};
      screen->copyBufferToScreen(console->accessFramebuffer());
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
    (void)hInst; (void)hInstPrev; (void)cmdline; (void)cmdshow;
    auto v = GetCommandLineW();
    int argc;
    wchar_t** argv = CommandLineToArgvW(v, &argc);
    return run(argc, argv);
  }

#else

  int main(int argc, char** argv)
  {
    wchar_t** argw = new wchar_t*[argc];
    for(int i = 0; i < argc; i++){
      auto sLen = strlen(argv[i])+1;
      argw[i] = new wchar_t[sLen];
      mbstowcs(argw[i], argv[i], sLen);
    }
    auto code = run(argc, argw);
    for(int i = 0; i < argc; i++){
      delete[] argw[i];
    }
    delete[] argw;
    return code;
  }

#endif
