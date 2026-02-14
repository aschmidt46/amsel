#pragma once
#include <mutex>
#include <filesystem>
#include "blockingconcurrentqueue.h"
#include <optional>
#include <string>
#include <vector>

enum MessageType{ // Toast Symbol
    MT_INFO,
    MT_SUCCESS,
    MT_WARNING,
    MT_ERROR
};

struct MessageStruct{
    MessageType type;
    std::optional<std::string> title;
    std::string content;
};

struct SettingsConfig{
    // Sound
    float volume = 1.0f;
    // Input
    // Controller 1, Primär- und Sekundärbelegung (Sekundär ist nur für Controller)
    std::vector<std::pair<int, int>> controller1;
    // Controller 2
    std::vector<std::pair<int, int>> controller2;

    // Joystick IDs für die beiden Controller
    int jidController1 = 0;
    int jidController2 = 1;
};

enum Action{
    AC_BUTTON_UP = 0,
    AC_BUTTON_DOWN = 1,
    AC_BUTTON_LEFT = 2,
    AC_BUTTON_RIGHT = 3,
    AC_BUTTON_A = 4,
    AC_BUTTON_B = 5,
    AC_BUTTON_START = 6,
    AC_BUTTON_SELECT = 7
};

extern std::mutex cvm;
extern std::filesystem::path exeDir;
extern moodycamel::BlockingConcurrentQueue<MessageStruct> messageQueue;
extern SettingsConfig globalConfig;
extern std::vector<int> connectedJoysticks;
