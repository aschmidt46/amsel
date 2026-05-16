#include "locale.h"
#include "nlohmann/json.hpp"
#include <fstream>
#include <iostream>

using json = nlohmann::json;

Locale::Locale(std::filesystem::path path){
    std::ifstream file(path);
    json js = json::parse(file);

    this->stringMap = std::vector<std::string>();

    this->name = js["displayName"].get<std::string>();

    const auto arr = js["strings"];

    if(arr.size() != NUM_STRINGS){
        std::cout << "Translation File has invalid number of strings" << std::endl;
        std::terminate();
    }

    for(const auto &el : arr){
        stringMap.push_back(el.get<std::string>());
    }

    file.close();
}

std::string Locale::getName() const{
    return this->name;
}

std::string Locale::getTranslation(LocalizedString id) const{
    return this->stringMap[id];
}
