#pragma once
#include <glad/gl.h>
#include <vector>
#include "glm_replacement.h"

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

// Public Domain CRT Styled Scan-Line Shader von Timothy Lottes
constexpr const char* fixingPixelArt = 
"#version 460\n"
"uniform vec3      iResolution;           // viewport resolution (in pixels)\n"
"uniform sampler2D tex;             // input channel. XX = 2D/Cube\n"
"in vec2 TexCoord;\n"
"out vec4 FragColor;\n"
"  #define res (vec2(256.0/1.0,240.0/1.0))\n"
"float hardScan=-8.0;\n"
"float hardPix=-3.0;\n"
"vec2 warp=vec2(1.0/32.0,1.0/24.0); \n"
"float maskDark=0.5;\n"
"float maskLight=1.5;\n"
"float ToLinear1(float c){return(c<=0.04045)?c/12.92:pow((c+0.055)/1.055,2.4);}\n"
"vec3 ToLinear(vec3 c){return vec3(ToLinear1(c.r),ToLinear1(c.g),ToLinear1(c.b));}\n"
"float ToSrgb1(float c){return(c<0.0031308?c*12.92:1.055*pow(c,0.41666)-0.055);}\n"
"vec3 ToSrgb(vec3 c){return vec3(ToSrgb1(c.r),ToSrgb1(c.g),ToSrgb1(c.b));}\n"
"vec3 Fetch(vec2 pos,vec2 off){\n"
"  pos=floor(pos*res+off)/res;\n"
"  if(max(abs(pos.x-0.5),abs(pos.y-0.5))>0.5){return vec3(0.0,0.0,0.0);}\n"
"  vec3 samp = texture(tex,pos.xy,-16.0).rgb;\n"
"  return ToLinear(samp);}\n"
"vec2 Dist(vec2 pos){pos=pos*res;return -((pos-floor(pos))-vec2(0.5));}\n"
"float Gaus(float pos,float scale){return exp2(scale*pos*pos);}\n"
"vec3 Horz3(vec2 pos,float off){\n"
"  vec3 b=Fetch(pos,vec2(-1.0,off));\n"
"  vec3 c=Fetch(pos,vec2( 0.0,off));\n"
"  vec3 d=Fetch(pos,vec2( 1.0,off));\n"
"  float dst=Dist(pos).x;\n"
"  float scale=hardPix;\n"
"  float wb=Gaus(dst-1.0,scale);\n"
"  float wc=Gaus(dst+0.0,scale);\n"
"  float wd=Gaus(dst+1.0,scale);\n"
"  return (b*wb+c*wc+d*wd)/(wb+wc+wd);}\n"
"vec3 Horz5(vec2 pos,float off){\n"
"  vec3 a=Fetch(pos,vec2(-2.0,off));\n"
"  vec3 b=Fetch(pos,vec2(-1.0,off));\n"
"  vec3 c=Fetch(pos,vec2( 0.0,off));\n"
"  vec3 d=Fetch(pos,vec2( 1.0,off));\n"
"  vec3 e=Fetch(pos,vec2( 2.0,off));\n"
"  float dst=Dist(pos).x;\n"
"  float scale=hardPix;\n"
"  float wa=Gaus(dst-2.0,scale);\n"
"  float wb=Gaus(dst-1.0,scale);\n"
"  float wc=Gaus(dst+0.0,scale);\n"
"  float wd=Gaus(dst+1.0,scale);\n"
"  float we=Gaus(dst+2.0,scale);\n"
"  return (a*wa+b*wb+c*wc+d*wd+e*we)/(wa+wb+wc+wd+we);}\n"
"float Scan(vec2 pos,float off){\n"
"  float dst=Dist(pos).y;\n"
"  return Gaus(dst+off,hardScan);}\n"
"vec3 Tri(vec2 pos){\n"
"  vec3 a=Horz3(pos,-1.0);\n"
"  vec3 b=Horz5(pos, 0.0);\n"
"  vec3 c=Horz3(pos, 1.0);\n"
"  float wa=Scan(pos,-1.0);\n"
"  float wb=Scan(pos, 0.0);\n"
"  float wc=Scan(pos, 1.0);\n"
"  return a*wa+b*wb+c*wc;}\n"
"vec2 Warp(vec2 pos){\n"
"  pos=pos*2.0-1.0;    \n"
"  pos*=vec2(1.0+(pos.y*pos.y)*warp.x,1.0+(pos.x*pos.x)*warp.y);\n"
"  return pos*0.5+0.5;}\n"
"vec3 Mask(vec2 pos){\n"
"  pos.x+=pos.y*3.0;\n"
"  vec3 mask=vec3(maskDark,maskDark,maskDark);\n"
"  pos.x=fract(pos.x/6.0);\n"
"  if(pos.x<0.333)mask.r=maskLight;\n"
"  else if(pos.x<0.666)mask.g=maskLight;\n"
"  else mask.b=maskLight;\n"
"  return mask;}    \n"
"float Bar(float pos,float bar){pos-=bar;return pos*pos<4.0?0.0:1.0;}\n"
"void main(){\n"
"  vec2 TexCoordSized = vec2(TexCoord.x * iResolution.x, TexCoord.y * iResolution.y);\n"
"  vec2 pos=Warp(TexCoordSized/iResolution.xy+vec2(0.0,0.0));\n"
"  FragColor = vec4(ToSrgb(Tri(pos)*Mask(TexCoordSized)), 1.0);\n"
"}\n";




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

    unsigned int screenTexture, vao, ssbo, uvssbo, basicShader, crtShader;

    int width = 256, height = 240; // Wird immer durch Einstellungen überschrieben

    int iResolutionX = width;
    int iResolutionY = height;

    void setQSize(float x0, float x1, float y0, float y1);

    void recreateTexture(int width, int height);

    public:
    Screen();
    ~Screen(){

    };

    vec4 computeRect(int x, int y);
    void onSwitchConsole();
    void present();
    void setPixelColor(int x, int y, vec3 c);
    void copyBufferToScreen(const float* buffer);   //float array, jeder Wert ist eine Farbe zwischen 0 und 1
    void updateFramebufferSize(int w, int h);
};
