#include "console.h"
#include "nes_implementation.h"
#include "cgb_implementation.h"
#include "../framework/screen.h"
#include "../framework/global.h"

void createConsole(const char *path)
{
    std::lock_guard lock{consoleLock};
    std::string filename(path);
    if(filename.substr(filename.find_last_of(".") + 1) == "nes"){
      new (&*console) NesImplementation();
      console->load(filename.c_str());
    }
    else if(filename.substr(filename.find_last_of(".") + 1) == "gb" || filename.substr(filename.find_last_of(".") + 1) == "gbc"){
      new (&*console) CgbImplementation(filename.c_str());
    }
    else return;

    screen->onSwitchConsole();
}