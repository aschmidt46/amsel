#pragma once

struct vec3{
    float r;
    float g;
    float b;

    vec3(float r, float g, float b): r(r), g(g), b(b) {};
};

struct vec4{
    float x;
    float y;
    float z;
    float w;

    vec4(float x, float y, float z, float w): x(x), y(y), z(z), w(w) {};
};
