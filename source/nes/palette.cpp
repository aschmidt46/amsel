#include "palette.h"
#include <fstream>
#include <vector>
#include <iostream>

Palette::Palette(const char *path) : Palette()
{
    std::ifstream stream(path, std::ios::in | std::ios::binary);
    std::vector<uint8_t> contents((std::istreambuf_iterator<char>(stream)), std::istreambuf_iterator<char>());
    
    if(contents.size()==0) return;

    if(contents.size() == 192){
        for(int i = 0; i < contents.size(); i++){
            colors[i] = contents[i];
        }
    }
    else{
        for(int i = 0; i < 192; i++){
            colors[i] = defPalette[i];
        }
    }
    stream.close();
}

Palette::Palette()
{
    for(int i = 0; i < 192; i++){
        colors[i] = defPalette[i];
    }
}

vec3 Palette::getColor(uint8_t index)
{
    if(3*index + 2 > 192) {
        std::cout << "Palette-Index außer Reichweite!" << std::endl;
        throw;
    }
    float r,g,b;
    r = colors[3*index];
    g = colors[3*index+1];
    b = colors[3*index+2];
    return vec3(r/255.0f,g/255.0f,b/255.0f);
}
