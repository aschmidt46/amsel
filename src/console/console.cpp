#include "console.h"
#ifdef BUILD_GBA
#include "gba_implementation.h"
#endif
#ifdef BUILD_NDS
#include "nds_implementation.h"
#endif
#ifdef BUILD_NES
#include "nes_implementation.h"
#endif
#ifdef BUILD_CGB
#include "cgb_implementation.h"
#endif
#include "dummy_implementation.h"
#ifdef BUILD_DESKTOP
    #include "../framework/screen.h"
#endif
#include "../framework/global.h"
#include "framework/file_io.h"

// Falls ich Doppelbelegung will, hier Vektor zurückgeben?
std::pair<AmselControlAction, int> calculateAmselAction(ControlOptions *controls, unsigned int key, bool isGamepadKey)
{
  const auto &bindings = controls->controllers;
  for(size_t c = 0; c < controls->numControllers; c++){
    for(size_t i = 0; i < controls->numButtons; i++){
      const auto storedVal = isGamepadKey ? bindings[c][i].second : bindings[c][i].first;
      if((unsigned int)storedVal == key){
        return {controls->mapping[i], c};
      }
    }
  }

  return {AmselControlAction_Invalid, 0};
}

void createConsole(const char *path)
{
    std::lock_guard lock{consoleLock};
    std::string filename(path);
    delete console;
    console = nullptr;
    #ifdef BUILD_NES
    if(filename.substr(filename.find_last_of(".") + 1) == "nes"){
        console = new NesImplementation();
        console->load(filename.c_str());
    }
    #endif
    #ifdef BUILD_CGB
    if(filename.substr(filename.find_last_of(".") + 1) == "gb" || filename.substr(filename.find_last_of(".") + 1) == "gbc"){
        console = new CgbImplementation(filename.c_str());
    }
    #endif
    #ifdef BUILD_GBA
    if(filename.substr(filename.find_last_of(".") + 1) == "gba"){
        console = new GbaImplementation(filename.c_str());
    }
    #endif
    #ifdef BUILD_NDS
    if(filename.substr(filename.find_last_of(".") + 1) == "nds"){
        console = new NDSImplementation(filename.c_str());
    }
    #endif
    if(console == nullptr){
        console = new DummyImplementation();
    }

    #ifdef BUILD_DESKTOP
        screen->onSwitchConsole();
    #endif
}

std::unique_ptr<Console> createConsoleFromData(std::string filename, std::vector<uint8_t> rom)
{
    std::unique_ptr<Console> c = std::make_unique<DummyImplementation>();
    #ifdef BUILD_NES
    if(filename.substr(filename.find_last_of(".") + 1) == "nes"){
        c = std::make_unique<NesImplementation>(rom);
    }
    #endif
    #ifdef BUILD_CGB
    if(filename.substr(filename.find_last_of(".") + 1) == "gb" || filename.substr(filename.find_last_of(".") + 1) == "gbc"){
        c = std::make_unique<CgbImplementation>(rom);
    }
    #endif
    #ifdef BUILD_GBA
    if(filename.substr(filename.find_last_of(".") + 1) == "gba"){
        c = std::make_unique<GbaImplementation>(rom);
    }
    #endif
    #ifdef BUILD_NDS
    if(filename.substr(filename.find_last_of(".") + 1) == "nds"){
        c = std::make_unique<NDSImplementation>(rom);
    }
    #endif
    c->setName(filename);
    return c;
}

void Console::loadMappings()
{
    #ifdef BUILD_DESKTOP
    auto name = getConsoleName();
    auto controls = getControlOptions();
    FileIO::getInstance().loadSystemControls(name, controls);
    #endif
}

Console::Console(const char *path)
{
    std::filesystem::path p(path);
    this->loadedGame = p.filename().string();
}

void Console::setKeyInt(int controller, int key, int action)
{
    const auto p = calculateAmselAction(getControlOptions(), key, false);
    setKey(p.second, p.first, action);
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
  register_vector<std::string>("vec_str");
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
    .function("setKey", &Console::setKeyInt)
    .function("getGameTitle", &Console::getGameTitle)
    .function("canSave", &Console::canSave)
    .function("getSaveData", &Console::getSaveData)
    .function("setVolume", &Console::setVolume)
    .function("getRequiredFiles", &Console::getRequiredFiles)
    .function("loadSpecialFile", &Console::loadSpecialFile)
    .function("setHalt", &Console::setHalt)
    .function("initializeControls", &Console::initializeControls)
    ;

//   smart_ptr<std::shared_ptr<Console>>("CXXConsole");
  function("createConsole", &createConsoleFromData);
}

#endif

std::vector<SystemOption> DummyImplementation::options = {};

ControlOptions DummyImplementation::controls = {};
