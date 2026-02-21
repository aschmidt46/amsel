#include "screen.h"
#include <iostream>
#include "global.h"

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
    
    unsigned int vertex, fragmentBasic, fragmentCRT;
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

    fragmentBasic = glCreateShader(GL_FRAGMENT_SHADER);

    glShaderSource(fragmentBasic, 1, &fs, NULL);
    glCompileShader(fragmentBasic);
    glGetShaderiv(fragmentBasic, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(fragmentBasic, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << infoLog << std::endl;
        throw;
    };

    fragmentCRT = glCreateShader(GL_FRAGMENT_SHADER);

    glShaderSource(fragmentCRT, 1, &fixingPixelArt, NULL);
    glCompileShader(fragmentCRT);
    glGetShaderiv(fragmentCRT, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(fragmentCRT, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << infoLog << std::endl;
        throw;
    };

    basicShader = glCreateProgram();
    glAttachShader(basicShader, vertex);
    glAttachShader(basicShader, fragmentBasic);
    glLinkProgram(basicShader);

    glGetProgramiv(basicShader, GL_LINK_STATUS, &success);
    if (!success)
    {
        glGetProgramInfoLog(basicShader, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
        throw;
    }

    crtShader = glCreateProgram();
    glAttachShader(crtShader, vertex);
    glAttachShader(crtShader, fragmentCRT);
    glLinkProgram(crtShader);

    glGetProgramiv(crtShader, GL_LINK_STATUS, &success);
    if (!success)
    {
        glGetProgramInfoLog(crtShader, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
        throw;
    }

    glDeleteShader(vertex);
    glDeleteShader(fragmentBasic);
    glDeleteShader(fragmentCRT);

    glUseProgram(basicShader);
    
    glTextureStorage2D(screenTexture, 1, GL_RGB8, screenWidth, screenHeight);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); // Für CRT Shader
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glNamedBufferData(ssbo, quad.size()*sizeof(float), quad.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, ssbo);
    
    glNamedBufferData(uvssbo, quad.size()*sizeof(float), quad.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, uvssbo);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, uvssbo);
    glUniform3f(glGetUniformLocation(basicShader, "iResolution"), iResolutionX, iResolutionY, 1);
    glUseProgram(crtShader);
    glUniform3f(glGetUniformLocation(crtShader, "iResolution"), iResolutionX, iResolutionY, 1);
    
}

void Screen::present()
{
    if(globalConfig.useCRTShader){
        glUseProgram(crtShader);
    }
    else{
        glUseProgram(basicShader);
    }
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

// Abbildung des NES-Seitenverhätnisses auf den Bildschirm
glm::vec4 Screen::computeRect(int x, int y){
    float nesX = 256.0;
    float nesY = 240;
    float nesAspect = nesX / nesY;
    float aspect = (float)y/(float)x;
    float cX = (float)x / 2.0;
    float cY = (float)y / 2.0;
    float h = 0;
    float w = 0;
    
    float x0=0, x1=1, y0=0, y1=1;
    if(aspect < nesAspect){ // Breiter als Nes
        h = y;
        w = h * nesAspect;
    }
    else{ // Schmaler als Nes
        w = x;
        h = x * (nesY / nesX);
    }
    x0 = cX - (w/2);
    x1 = cX + (w/2);
    y0 = cY - (h/2);
    y1 = cY + (h/2);

    iResolutionX = w;
    iResolutionY = h;

    // Skalierung auf [0,1]

    x0 /= x;
    x1 /= x;
    y0 /= y;
    y1 /= y;

    // Skalierung auf NDC im Shader

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
    glUniform3f(glGetUniformLocation(basicShader, "iResolution"), iResolutionX, iResolutionY, 1);
    glUniform3f(glGetUniformLocation(crtShader, "iResolution"), iResolutionX, iResolutionY, 1);
}
