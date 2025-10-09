#pragma once
#include <GL/glew.h>
#include <vector>
#include <glm/glm.hpp>

constexpr const char* vs = 
    "#version 460\n"
    "layout (std430, binding = 0) buffer quadBuffer"
    "{"
        "vec2 quad[];"
    "};\n"
    "layout (std430, binding = 1) buffer uvBuffer"
    "{"
        "vec2 uv[];"
    "};\n"
    "out vec2 TexCoord;\n"
    "void main(){"
        "vec2 quadCoord = quad[gl_VertexID];"
        "vec2 uvCoord = uv[gl_VertexID];"
        "TexCoord = vec2(uvCoord.x, 1-uvCoord.y);"
        "vec2 ndc = vec2(2 * quadCoord.x - 1, 2 * quadCoord.y - 1);"
        "gl_Position = vec4(ndc.xy, 0, 1.0);"
    "}";

constexpr const char* fs = 
    "#version 460\n"
    "out vec4 FragColor;\n"
    "uniform sampler2D tex;\n"
    "in vec2 TexCoord;\n"
    "void main(){"
        "FragColor = texture(tex, TexCoord);\n"
    "}";

constexpr const int screenWidth = 256;
constexpr const int screenHeight = 240;

const std::vector<float> quad{      // UVs      // VertexArrays mit 6 Instanzen zeichnen, dann mit gl_InstanceID Position bestimmen
    0,0,
    1,0,
    0,1,

    1,0,
    1,1,
    0,1
};

class Screen{

    std::vector<float> qSize = quad;

    unsigned int screenTexture, vao, ssbo, uvssbo, shaderProgram;

    int width = 256, height = 240;

    void setQSize(float x0, float x1, float y0, float y1);

    public:
    Screen();
    ~Screen(){

    };

    void present();
    void setPixelColor(int x, int y, glm::vec3 c);
    void copyBufferToScreen(float* buffer);   //float array, jeder Wert ist eine Farbe zwischen 0 und 1
    void updateFramebufferSize(int w, int h);
};
