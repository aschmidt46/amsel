#include "palette.h"
#include <fstream>
#include <vector>
#include <iostream>

Palette::Palette(const char *path) : Palette()
{
    std::ifstream stream(path, std::ios::in | std::ios::binary);
    std::vector<uint8_t> contents((std::istreambuf_iterator<char>(stream)), std::istreambuf_iterator<char>());
    
    if(contents.size()==0) return;
    else delete[] colors;

    colors = new uint8_t[contents.size()];
    pSize = contents.size();
    for(int i = 0; i < contents.size(); i++){
        colors[i] = contents[i];
    }
    stream.close();
}

Palette::Palette()
{
    colors = new uint8_t[pSize];
    for(int i = 0; i < pSize; i++){
        colors[i] = defPalette[i];
    }
}

glm::vec3 Palette::getColor(uint8_t index)
{
    if(3*index + 2 > pSize) {
        std::cout << "Palette-Index außer Reichweite!" << std::endl;
        throw;
    }
    float r,g,b;
    r = colors[3*index];
    g = colors[3*index+1];
    b = colors[3*index+2];
    return glm::vec3(r/255.0f,g/255.0f,b/255.0f);
}
