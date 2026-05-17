#pragma once
#include <string>
#include <vector>
#include <filesystem>

enum LocalizedString{
    ChooseRom = 0,
    RomFilter = 1,

    // Gamepad Strings
    GamepadKeyBack = 2,
    DpadUp = 3,
    DpadDown = 4,
    DpadLeft = 5,
    DpadRight = 6,

    // Tastatur Strings
    KeySpace = 7,
    KeyEsc = 8,
    KeyEnter = 9,
    KeyTab = 10,
    KeyBackspace = 11,
    KeyInsert = 12,
    KeyDelete = 13,
    KeyRight = 14,
    KeyLeft = 15,
    KeyDown = 16,
    KeyUp = 17,
    KeyPageUp = 18,
    KeyPageDown = 19,
    KeyHome = 20,
    KeyEnd = 21,
    KeyCapsLock = 22,
    KeyScrollLock = 23,
    KeyNumLock = 24,
    KeyPrint = 25,
    KeyPause = 26,
    KeyF1 = 27,
    KeyF2 = 28,
    KeyF11 = 29,
    KeyNumEnter = 30,
    KeyLShift = 31,
    KeyLCRTL = 32,
    KeyLAlt = 33,
    KeyLSuper = 34,
    KeyRShift = 35,
    KeyRCRTL = 36,
    KeyRAlt = 37,
    KeyRSuper = 38,
    KeyMenu = 39,

    DebuggerJump = 40,
    DebuggerAddress = 41,
    DebuggerRead1Byte = 42,
    DebuggerRead2Byte = 43,
    DebuggerHex = 44,
    DebuggerOpcode = 45,
    DebuggerBin = 46,
    DebuggerAdd = 47,
    DebuggerAddOp = 48,
    DebuggerBreakOp = 49,
    DebuggerBreakAddr = 50,
    DebuggerDebugger = 51,
    DebuggerBreakButton = 52,
    DebuggerStep = 53,
    DebuggerRegister = 54,
    DebuggerRam = 55,
    DebuggerBreakpoints = 56,

    TestOutput = 57,
    TestOutputStartAddress = 58,

    Controls = 59,
    Controller1 = 60,
    Controller2 = 61,
    SAction = 62,
    BindingKeyboard = 63,
    BindingGamepad = 64,
    BindingDpadUp = 65,
    BindingDpadDown = 66,
    BindingDpadLeft = 67,
    BindingDpadRight = 68,
    BindingAButton = 69,
    BindingBButton = 70,
    BindingStartButton = 71,
    BindingSelectButton = 72,
    GamepadColon = 73,
    PressButtonPrompt = 74,

    MenuFile = 75,
    FileLoad = 76,

    MenuSettings = 77,
    SettingsControls = 78,
    SettingsFullscreen = 79,
    SettingsCRTShader = 80,
    SettingsSound = 81,

    MenuDebugger = 82,
    DebuggerDBItem = 83,
    DebuggerText = 84,

    // file_io.cpp
    SaveFileCreated = 85,

    // nes.cpp
    GameError = 86,
    MapperNotSupported = 87,

    //6502.cpp
    EmulatorError = 88,
    UnimplementedInstruction = 89,
    UnsafeInstruction = 90,
    EmulatorWarning = 91,
    BRKExecuted = 92,
    SettingsLanguage = 93,

    AboutTitle = 94,
    AboutContent = 95,
    AboutSystems= 96,
    AboutSystem1 = 97,
    AboutSystem2 = 98,
    AboutSource = 99,

};

constexpr size_t NUM_STRINGS = 100;

class Locale{
    private:
    std::vector<std::string> stringMap;
    std::string name;
    std::string code;
    public:
    Locale(std::filesystem::path path);
    Locale() = default;
    std::string getName() const;
    std::string getCode() const;
    std::string getTranslation(LocalizedString id) const;
};
