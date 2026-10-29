#include "file_io.h"
#include <fstream>
#include <variant>
#include "console/console.h"
#include "inicpp.h"
#include "common.h"

FileIO::FileIO()
{
    saveDirectory = exeDir / "saves";
    workDirectory = exeDir;

    if (!std::filesystem::is_directory(saveDirectory) || !std::filesystem::exists(saveDirectory)) {
        std::filesystem::create_directory(saveDirectory);
    }

}

void initSettings(SettingsConfig &c){
  
//   setDefaultBindings(c);

  c.directory = exeDir.generic_string();

}

void FileIO::writeDefaultSettings(const std::string &fileName, int posX, int posY)
{
    (void)fileName;
    SettingsConfig c;
    c.posX = posX;
    c.posY = posY;
    initSettings(c);

    saveSettings(c);
}

FileIO &FileIO::getInstance()
{
    static FileIO instance;

    return instance;
}

bool FileIO::createSave(std::string romName)
{
    if(!std::filesystem::exists(saveDirectory / (romName + ".sav"))){
        std::ofstream ofs(saveDirectory / (romName + ".sav"));
        ofs.close();
        int stem = romName.find_last_of('.');
        MessageStruct m = {.type = MT_SUCCESS, .title = romName.substr(0, stem), .content = locale.getTranslation(SaveFileCreated)};
        messageQueue.enqueue(m);
        return true;
    }

    return false;
}

void FileIO::saveData(std::string romName, uint8_t *data, int size)
{
    std::ofstream f(saveDirectory / (romName + ".sav"), std::ios_base::binary | std::ios_base::out | std::ios_base::trunc);
    f.write((char*)data, size);
    f.close();
}

void FileIO::loadSave(std::string romName, uint8_t *destination, int size)
{
    std::ifstream f(saveDirectory / (romName + ".sav"), std::ios_base::binary | std::ios_base::in);
    f.read((char*)destination, size);
    f.close();
}

void FileIO::loadSave(std::string romName, std::vector<uint8_t> &destination, int size)
{
    size_t usize = size;
    if(size == 0){
        usize = std::filesystem::file_size(saveDirectory / (romName + ".sav"));
        destination.resize(usize);
    }
    loadSave(romName, destination.data(), (int)usize);
}

SettingsConfig FileIO::loadSettings(int posX, int posY)
{
    if(!std::filesystem::exists(workDirectory / "settings.ini")){
        writeDefaultSettings((workDirectory / "settings.ini").string(), posX, posY);
    }

    ini::IniFile ini((workDirectory / "settings.ini").string());

    return SettingsConfig{
            .directory = ini["General"]["WorkDirectory"].as<std::string>(),
            .language = ini["General"]["Language"].as<std::string>(),
            .posX = ini["Display"]["WindowPosX"].as<int>(),
            .posY = ini["Display"]["WindowPosY"].as<int>(),
            .sizeX = ini["Display"]["WindowSizeX"].as<int>(),
            .sizeY = ini["Display"]["WindowSizeY"].as<int>(),
            .maximize = ini["Display"]["Maximize"].as<bool>(),
            .useCRTShader = ini["Display"]["CRTShader"].as<bool>(),
			.volume = ini["Sound"]["Volume"].as<float>(),
            .unmute = ini["Sound"]["Unmute"].as<bool>(),
	};
}

void FileIO::saveSettings(const SettingsConfig &config)
{
    ini::IniFile ini;

    ini["General"]["WorkDirectory"] = config.directory;
    ini["General"]["Language"] = config.language;
    ini["Display"]["WindowPosX"] = config.posX;
    ini["Display"]["WindowPosY"] = config.posY;
    ini["Display"]["WindowSizeX"] = config.sizeX;
    ini["Display"]["WindowSizeY"] = config.sizeY;
    ini["Display"]["Maximize"] = config.maximize;
    ini["Display"]["CRTShader"] = config.useCRTShader;
    ini["Sound"]["Volume"] = config.volume;
    ini["Sound"]["Unmute"] = config.unmute;

    ini.save((workDirectory / "settings.ini").string());
}

void FileIO::loadSystemControls(const std::string &consoleName, ControlOptions *controls)
{
    std::string fileName = consoleName;
    fileName.erase(remove_if(fileName.begin(), fileName.end(), isspace), fileName.end());
    fileName += "_Controls.ini";

    if(!std::filesystem::exists(workDirectory / fileName)){
        return;
    }

    ini::IniFile ini((workDirectory / fileName).string());

    for(size_t j = 0; j < controls->numControllers; j++){
        controls->joysticks[j] = ini["Controller"+std::to_string(j)]["Gamepad_ID"].as<int>();
        for(size_t i = 0; i < controls->numButtons; i++){
            controls->controllers[j][i].first = ini["Controller"+std::to_string(j)]["Action_"+std::to_string(i)+"_primary"].as<int>();
            controls->controllers[j][i].second = ini["Controller"+std::to_string(j)]["Action_"+std::to_string(i)+"_secondary"].as<int>();
        }
    }
}

void FileIO::saveSystemControls(const std::string &consoleName, ControlOptions *controls)
{
    ini::IniFile ini;

    for(size_t j = 0; j < controls->numControllers; j++){
        ini["Controller"+std::to_string(j)]["Gamepad_ID"] = controls->joysticks[j];
        for(size_t i = 0; i < controls->numButtons; i++){
            ini["Controller"+std::to_string(j)]["Action_"+std::to_string(i)+"_primary"] = controls->controllers[j][i].first;
            ini["Controller"+std::to_string(j)]["Action_"+std::to_string(i)+"_secondary"] = controls->controllers[j][i].second;
        }
    }

    std::string fileName = consoleName;
    fileName.erase(remove_if(fileName.begin(), fileName.end(), isspace), fileName.end());
    fileName += "_Controls.ini";
    ini.save((workDirectory / fileName).string());
}

void FileIO::saveSystemSettings(const std::vector<std::pair<std::string, std::vector<SystemOption>*>> &options){
    for(const auto &console : options){

        ini::IniFile ini;

        auto visitor = overload{
            [&](const RequiredFile &f){
                std::string optName = f.id;
                optName.erase(remove_if(optName.begin(), optName.end(), isspace), optName.end());
                ini["General"][optName] = f.path;        
            },
            [&](const Toggle &t){
                std::string optName = t.id;
                optName.erase(remove_if(optName.begin(), optName.end(), isspace), optName.end());
                ini["General"][optName] = t.value;
            },
            [&](const CustomMenu &m){

            }
        };

        for(const auto &option : *console.second){
            std::visit(visitor, option);
        }
        std::string fileName = console.first;
        fileName.erase(remove_if(fileName.begin(), fileName.end(), isspace), fileName.end());
        fileName += ".ini";
        ini.save((workDirectory / fileName).string());
    }
}

void FileIO::loadSystemSettings(const std::vector<std::pair<std::string, std::vector<SystemOption>*>> &options){
    for(auto &console : options){
        std::string fileName = console.first;
        fileName.erase(remove_if(fileName.begin(), fileName.end(), isspace), fileName.end());
        fileName += ".ini";

        if(!std::filesystem::exists(workDirectory / fileName)){
            continue;
        }

        ini::IniFile ini((workDirectory / fileName).string());

        auto visitor = overload{
            [&](RequiredFile &f){
                std::string optName = f.id;
                optName.erase(remove_if(optName.begin(), optName.end(), isspace), optName.end());
                f.path = ini["General"][optName].as<std::string>();
            },
            [&](Toggle &t){
                std::string optName = t.id;
                optName.erase(remove_if(optName.begin(), optName.end(), isspace), optName.end());
                t.value = ini["General"][optName].as<bool>();
            },
            [&](CustomMenu &m){

            }
        };

        for(auto &option : *console.second){
            std::visit(visitor, option);
        }
    }
}
