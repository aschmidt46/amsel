#pragma once

#include <filesystem>
#include "global.h"


class FileIO{
    FileIO();
    static FileIO* instance;
    
    
    std::filesystem::path workDirectory;
    std::filesystem::path saveDirectory;

    void writeDefaultSettings(const std::string &fileName , int posX, int posY);

    public:
    static FileIO& getInstance();

    // Rückgabewert true: Neu
    bool createSave(std::string romName);
    void saveData(std::string romName, uint8_t* data, int size);
    void loadSave(std::string romName, uint8_t* destination, int size);

    SettingsConfig loadSettings(int posX, int posY);
    void saveSettings(const SettingsConfig &config);
};
