#include "global.h"
#include "common.h"
#include <emscripten.h>
#include <iostream>
#include <emscripten/bind.h>

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


int main(){
    setDefaultBindings(globalConfig);
    std::cout << "AMSEL initialized." << std::endl;
    std::cout << "unsigned long long Bytes: " << sizeof(1ull) << std::endl;
    return 0;
}
