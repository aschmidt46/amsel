#include "console.h"
#include "nes_implementation.h"
#include "cgb_implementation.h"
#include "dummy_implementation.h"
#include "../framework/screen.h"
#include "../framework/global.h"

void createConsole(const char *path)
{
    std::lock_guard lock{consoleLock};
    std::string filename(path);
    delete console;
    if(filename.substr(filename.find_last_of(".") + 1) == "nes"){
        console = new NesImplementation();
        console->load(filename.c_str());
    }
    else if(filename.substr(filename.find_last_of(".") + 1) == "gb" || filename.substr(filename.find_last_of(".") + 1) == "gbc"){
        console = new CgbImplementation(filename.c_str());
    }
    else{
        console = new DummyImplementation();
    }

    screen->onSwitchConsole();
}

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
