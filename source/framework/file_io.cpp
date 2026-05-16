#include "file_io.h"
#include <fstream>
#include "inicpp.h"

FileIO::FileIO()
{
    saveDirectory = exeDir / "saves";
    workDirectory = exeDir;

    if (!std::filesystem::is_directory(saveDirectory) || !std::filesystem::exists(saveDirectory)) {
        std::filesystem::create_directory(saveDirectory);
    }

}

void initSettings(SettingsConfig &c){
  c.controller1 = std::vector<std::pair<int, int>>(8, {0,0});
  c.controller2 = std::vector<std::pair<int, int>>(8, {0,0});
  // Standard Belegung Controller 1
  c.controller1[0].first = 265;
  c.controller1[1].first = 264;
  c.controller1[2].first = 263;
  c.controller1[3].first = 262;
  c.controller1[4].first = 83;
  c.controller1[5].first = 65;
  c.controller1[6].first = 257;
  c.controller1[7].first = 259;

  c.controller1[0].second = 23;
  c.controller1[1].second = 25;
  c.controller1[2].second = 26;
  c.controller1[3].second = 24;
  c.controller1[4].second = 12;
  c.controller1[5].second = 14;
  c.controller1[6].second = 19;
  c.controller1[7].second = 18;

  // Standard Belegung Controller 2
  c.controller2[0].first = 328;
  c.controller2[1].first = 325;
  c.controller2[2].first = 324;
  c.controller2[3].first = 326;
  c.controller2[4].first = 88;
  c.controller2[5].first = 90;
  c.controller2[6].first = 335;
  c.controller2[7].first = 334;

  c.controller2[0].second = 23;
  c.controller2[1].second = 25;
  c.controller2[2].second = 26;
  c.controller2[3].second = 24;
  c.controller2[4].second = 12;
  c.controller2[5].second = 14;
  c.controller2[6].second = 19;
  c.controller2[7].second = 18;

  c.directory = exeDir.generic_string();

}

void FileIO::writeDefaultSettings(const std::string &fileName, int posX, int posY)
{
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

SettingsConfig FileIO::loadSettings(int posX, int posY)
{
    if(!std::filesystem::exists(workDirectory / "settings.ini")){
        writeDefaultSettings((workDirectory / "settings.ini").string(), posX, posY);
    }

    ini::IniFile ini((workDirectory / "settings.ini").string());

    auto c1 = std::vector<std::pair<int, int>>(8);
    auto c2 = std::vector<std::pair<int, int>>(8);

    for(int i = 0; i < 8; i++){
        c1[i].first = ini["Controller1"]["Action_"+std::to_string(i)+"_primary"].as<int>();
        c1[i].second = ini["Controller1"]["Action_"+std::to_string(i)+"_secondary"].as<int>();
    }

    for(int i = 0; i < 8; i++){
        c2[i].first = ini["Controller2"]["Action_"+std::to_string(i)+"_primary"].as<int>();
        c2[i].second = ini["Controller2"]["Action_"+std::to_string(i)+"_secondary"].as<int>();
    }

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
            .controller1 = c1,
            .controller2 = c2,
            .jidController1 = ini["Devices"]["Gamepad_1"].as<int>(),
            .jidController2 = ini["Devices"]["Gamepad_2"].as<int>()
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

    for(int i = 0; i < 8; i++){
        ini["Controller1"]["Action_"+std::to_string(i)+"_primary"] = config.controller1[i].first;
        ini["Controller1"]["Action_"+std::to_string(i)+"_secondary"] = config.controller1[i].second;
    }
    for(int i = 0; i < 8; i++){
        ini["Controller2"]["Action_"+std::to_string(i)+"_primary"] = config.controller2[i].first;
        ini["Controller2"]["Action_"+std::to_string(i)+"_secondary"] = config.controller2[i].second;
    }

    ini["Devices"]["Gamepad_1"] = config.jidController1;
    ini["Devices"]["Gamepad_2"] = config.jidController2;

    ini.save((workDirectory / "settings.ini").string());
}
