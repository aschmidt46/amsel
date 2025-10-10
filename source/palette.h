#pragma once
#include <cstdint>
#include "glm/glm.hpp"

struct Palette{
    uint8_t* colors;
    size_t pSize;
    Palette(const char* path);
    ~Palette(){ delete[] colors; };
    glm::vec3 getColor(uint8_t index);
};
