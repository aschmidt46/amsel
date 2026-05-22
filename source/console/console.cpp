#include "console.h"
#include "nes_implementation.h"
#include "cgb_implementation.h"
#include "dummy_implementation.h"
#ifndef BUILD_WEB
    #include "../framework/screen.h"
#endif
#include "../framework/global.h"

#ifndef BUILD_WEB
void createConsole(const char *path)
{
    std::lock_guard lock{consoleLock};
    std::string filename(path);
    delete console;
    if(filename.substr(filename.find_last_of(".") + 1) == "nes"){
        console = new NesImplementation();
        console->load(filename.c_str());
    }
    #ifndef BUILD_WEB
    else if(filename.substr(filename.find_last_of(".") + 1) == "gb" || filename.substr(filename.find_last_of(".") + 1) == "gbc"){
        console = new CgbImplementation(filename.c_str());
    }
    #endif
    else{
        console = new DummyImplementation();
    }

    #ifndef BUILD_WEB
        screen->onSwitchConsole();
    #endif
}

#else

std::unique_ptr<Console> createConsole(std::string filename, std::vector<uint8_t> rom)
{
    std::unique_ptr<Console> c;
    if(filename.substr(filename.find_last_of(".") + 1) == "nes"){
        c = std::make_unique<NesImplementation>(rom);
    }
    else if(filename.substr(filename.find_last_of(".") + 1) == "gb" || filename.substr(filename.find_last_of(".") + 1) == "gbc"){
        c = std::make_unique<CgbImplementation>(rom);
    }
    else{
        c = std::make_unique<DummyImplementation>();
    }
    c->setName(filename);
    return c;
}

#endif

std::string Console::ihex(uintptr_t input)
{
    std::string str = std::format("{:x}", input);
    std::transform(str.begin(), str.end(), str.begin(), ::toupper);
    return str;
}

std::string Console::ihexNorm(std::string s, int n)
{
    while (s.size() < n)
        s = "0" + s;
    return s;
}

Console::Console(const char *path)
{
    std::filesystem::path p(path);
    this->loadedGame = p.filename().string();
}

std::string Console::getGameTitle()
{
    return this->loadedGame;
}

void Console::setName(std::string name)
{
    this->loadedGame = name;
}

void Console::setVolume(float v)
{
    this->volume = v;
}

#ifdef BUILD_WEB


EMSCRIPTEN_BINDINGS(ConsoleModule) {
  register_vector<uint8_t>("vec_u8");
  value_object<std::pair<double, double>>("pair_double")
        .field("first", &std::pair<double, double>::first)
        .field("second", &std::pair<double, double>::second)
        ;
//   value_object<const float*>("float_pointer");
  class_<Console>("CXXConsole")
    // .function("load", &Console::load)
    .function("clock", &Console::clock)
    .function("clockUntilSampleReady", &Console::clockUntilSampleReady)
    .function("accessFramebufferJS", &Console::accessFramebufferJS)
    .function("frameIsReady", &Console::frameIsReady)
    .function("audioSampleReady", &Console::audioSampleReady)
    .function("getSample", &Console::getSample)
    .function("isLoaded", &Console::isLoaded)
    .function("getX", &Console::getX)
    .function("getY", &Console::getY)
    .function("setController1Key", &Console::setController1Key)
    .function("setController2Key", &Console::setController2Key)
    .function("getGameTitle", &Console::getGameTitle)
    .function("canSave", &Console::canSave)
    .function("getSaveData", &Console::getSaveData)
    .function("setVolume", &Console::setVolume)
    ;

//   smart_ptr<std::shared_ptr<Console>>("CXXConsole");
  function("createConsole", &createConsole);
}

#endif
