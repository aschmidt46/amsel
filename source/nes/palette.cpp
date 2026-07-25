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
        for(size_t i = 0; i < contents.size(); i++){
            colors[i] = contents[i];
        }
    }
    else{
        for(size_t i = 0; i < 192; i++){
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

uint32_t Palette::getColor(uint8_t index)
{
    if(3*index + 2 > 192) {
        std::cout << "Palette-Index außer Reichweite!" << std::endl;
        throw;
    }
    uint32_t r,g,b;
    r = colors[3*index];
    g = colors[3*index+1];
    b = colors[3*index+2];
    #ifndef BUILD_LIBRETRO_CORE
    return (255u << 24) | (r) | (g << 8) | (b << 16);
    #else
    return (255u << 24) | (r << 16) | (g << 8) | (b);
    #endif
}
