#include "screen.h"
#include <iostream>
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

void Screen::setQSize(float x0, float x1, float y0, float y1)
{
    qSize[0] = x0;
    qSize[1] = y0;
    qSize[2] = x1;
    qSize[3] = y0;
    qSize[4] = x0;
    qSize[5] = y1;
    qSize[6] = x1;
    qSize[7] = y0;
    qSize[8] = x1;
    qSize[9] = y1;
    qSize[10] = x0;
    qSize[11] = y1;
}

Screen::Screen()
{
    glGenVertexArrays(1, &vao);
    glCreateBuffers(1, &ssbo);
    glCreateBuffers(1, &uvssbo);
    glGenTextures(1, &screenTexture);
    glBindTexture(GL_TEXTURE_2D, screenTexture);
    glActiveTexture(GL_TEXTURE0);
    
    unsigned int vertex, fragment;
    int success;
    char infoLog[512];

    vertex = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex, 1, &vs, NULL);
    glCompileShader(vertex);

    glGetShaderiv(vertex, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(vertex, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;
        throw;
    };

    fragment = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment, 1, &fs, NULL);
    glCompileShader(fragment);
    glGetShaderiv(fragment, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(fragment, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << infoLog << std::endl;
        throw;
    };

    shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertex);
    glAttachShader(shaderProgram, fragment);
    glLinkProgram(shaderProgram);

    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success)
    {
        glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
        throw;
    }

    glDeleteShader(vertex);
    glDeleteShader(fragment);

    glUseProgram(shaderProgram);

    glTextureStorage2D(screenTexture, 1, GL_RGB8, screenWidth, screenHeight);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glNamedBufferData(ssbo, quad.size()*sizeof(float), quad.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, ssbo);

    glNamedBufferData(uvssbo, quad.size()*sizeof(float), quad.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, uvssbo);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, uvssbo);

    // Testbild
    
}

void Screen::present()
{
    glClear(GL_COLOR_BUFFER_BIT);
    glBindVertexArray(vao);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, screenTexture);
    glDrawArrays(GL_TRIANGLES, 0, 6); // 6 Punkte
    glBindVertexArray(0);
}

void Screen::setPixelColor(int x, int y, glm::vec3 c)
{
    glTexSubImage2D(GL_TEXTURE_2D, 0, x, y, 1, 1, GL_RGB, GL_FLOAT, &c);
}

void Screen::copyBufferToScreen(float *buffer)
{
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, screenWidth, screenHeight, GL_RGB, GL_FLOAT, buffer);
}

glm::vec4 computeRect(int x, int y){
    float aspect = (float)y/(float)x;
    float x0=0, x1=1, y0=0, y1=1;
    if(aspect < 1){
        float aw = (1 - aspect) / 2;
        x0 = aw;
        x1 = aw + aspect;
    }
    else{
        aspect = (float)x/(float)y;
        float ah = (1 - aspect) / 2;
        y0 = ah;
        y1 = ah + aspect;
    }
    return glm::vec4(x0,x1,y0,y1);
}


// Konstantes Seitenverhältnis
void Screen::updateFramebufferSize(int w, int h)
{
    width = w;
    height = h;
    glm::vec4 xxyy = computeRect(w,h);
    
    setQSize(xxyy.x, xxyy.y, xxyy.z, xxyy.w);
    glNamedBufferData(ssbo, qSize.size()*sizeof(float), qSize.data(), GL_STATIC_DRAW);
    glViewport(0, 0, w, h);
}
