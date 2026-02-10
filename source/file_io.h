#pragma once

#include <filesystem>


class FileIO{
    FileIO();
    static FileIO* instance;
    
    
    std::filesystem::path workDirectory;
    bool message = false;

    public:
    static FileIO& getInstance();

    // Rückgabewert true: Neu
    bool createSave(std::string romName);
    void saveData(std::string romName, uint8_t* data, int size);
    void loadSave(std::string romName, uint8_t* destination, int size);

    bool shouldDisplayMessage();
};
