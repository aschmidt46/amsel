#include "file_io.h"
#include "global.h"
#include <fstream>

FileIO::FileIO()
{
    workDirectory = exeDir / "saves";

    if (!std::filesystem::is_directory(workDirectory) || !std::filesystem::exists(workDirectory)) {
        std::filesystem::create_directory(workDirectory);
    }

}

FileIO& FileIO::getInstance()
{
    static FileIO instance;

    return instance;
}

bool FileIO::createSave(std::string romName)
{
    if(!std::filesystem::exists(workDirectory / (romName + ".sav"))){
        std::ofstream ofs(workDirectory / (romName + ".sav"));
        ofs.close();
        message = true;
        return true;
    }

    return false;
}

void FileIO::saveData(std::string romName, uint8_t *data, int size)
{
    std::ofstream f(workDirectory / (romName + ".sav"), std::ios_base::binary | std::ios_base::out | std::ios_base::trunc);
    f.write((char*)data, size);
    f.close();
}

void FileIO::loadSave(std::string romName, uint8_t *destination, int size)
{
    std::ifstream f(workDirectory / (romName + ".sav"), std::ios_base::binary | std::ios_base::in);
    f.read((char*)destination, size);
    f.close();
}

bool FileIO::shouldDisplayMessage()
{
    if(message){
        message = false;
        return true;
    }
    return false;
}
