#include "global.h"
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
    std::cout << "Hallo Welt!" << std::endl;
    return 0;
}

using namespace emscripten;

float lerp(float a, float b, float t) {
    return (1 - t) * a + t * b;
}

EMSCRIPTEN_BINDINGS(my_module) {
    function("lerp", &lerp);
}
