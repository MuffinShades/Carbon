#pragma once
#include <iostream>
#include <cmath>
#include "msutil.hpp"

class vec2 {
public:
    float x = 0.0f, y = 0.0f;
    vec2(float x = 0.0f, float y = 0.0f) : x(x), y(y) {}

    //operators
    vec2 operator+(vec2 b);
    vec2 operator-(vec2 b);
    vec2 operator*(vec2 b);
    vec2 operator/(vec2 b);

    vec2 operator*(float s);
    vec2 operator/(float s);

    vec2 operator+(ivec2 b);
    vec2 operator-(ivec2 b);
    vec2 operator*(ivec2 b);
    vec2 operator/(ivec2 b);

    vec2 operator+(uvec2 b);
    vec2 operator-(uvec2 b);
    vec2 operator*(uvec2 b);
    vec2 operator/(uvec2 b);

    static float DotProd(vec2 a, vec2 b);
    static float CrossProd(vec2 a, vec2 b);

    float lenSqr();
    float len();
    void Normalize();

    vec2 GetNormal();
};

class vec3 {
public:
    float x = 0.0f, y = 0.0f, z = 0.0f;
    vec3(float x = 0.0f, float y = 0.0f, float z = 0.0f) : x(x), y(y), z(z) {}

    //operators
    vec3 operator+(vec3 b);
    vec3 operator-(vec3 b);
    vec3 operator*(vec3 b);
    vec3 operator/(vec3 b);

    vec3 operator*(float s);
    vec3 operator/(float s);

    vec3 operator+(ivec3 b);
    vec3 operator-(ivec3 b);
    vec3 operator*(ivec3 b);
    vec3 operator/(ivec3 b);

    vec3 operator+(uvec3 b);
    vec3 operator-(uvec3 b);
    vec3 operator*(uvec3 b);
    vec3 operator/(uvec3 b);

    static float DotProd(vec3 a, vec3 b);
    static vec3 CrossProd(vec3 a, vec3 b);

    float lenSqr();
    float len();

    static vec3 Normalize(vec3 v);

    vec3 GetNormal();
};

class vec4 {
public:
    float x = 0.0f, y = 0.0f, z = 0.0f, w = 0.0f;
    vec4(float x = 0.0f, float y = 0.0f, float z = 0.0f, float w = 0.0f) : x(x), y(y), z(z), w(w) {}

    //operators
    vec4 operator+(vec4 b);
    vec4 operator-(vec4 b);
    vec4 operator*(vec4 b);
    vec4 operator*(float s);
    vec4 operator/(float s);
    vec4 operator/(vec4 b);

    static float DotProd(vec4 a, vec4 b);

    float lenSqr();
    float len();

    static vec4 Normalize(vec4 v);
    vec4 GetNormal();
};

class ivec2 {
public:
    i32 x, y;
    ivec2(i32 x = 0, i32 y = 0) : x(x), y(y) {}

    ivec2 operator+(ivec2 b);
    ivec2 operator-(ivec2 b);
    ivec2 operator*(ivec2 b);
    ivec2 operator/(ivec2 b);

    ivec2 operator+(vec2 b);
    ivec2 operator-(vec2 b);
    ivec2 operator*(vec2 b);
    ivec2 operator/(vec2 b);

    ivec2 operator+(uvec2 b);
    ivec2 operator-(uvec2 b);
    ivec2 operator*(uvec2 b);
    ivec2 operator/(uvec2 b);
};

class uvec2 {
public:
    u32 x, y;
    uvec2(u32 x = 0, u32 y = 0) : x(x), y(y) {}

    uvec2 operator+(ivec2 b);
    uvec2 operator-(ivec2 b);
    uvec2 operator*(ivec2 b);
    uvec2 operator/(ivec2 b);

    uvec2 operator+(vec2 b);
    uvec2 operator-(vec2 b);
    uvec2 operator*(vec2 b);
    uvec2 operator/(vec2 b);

    uvec2 operator+(uvec2 b);
    uvec2 operator-(uvec2 b);
    uvec2 operator*(uvec2 b);
    uvec2 operator/(uvec2 b);
};

class ivec3 {
public:
    i32 x, y, z;
    ivec3(i32 x = 0, i32 y = 0, i32 z = 0) : x(x), y(y), z(z) {}

    ivec3 operator+(ivec3 b);
    ivec3 operator-(ivec3 b);
    ivec3 operator*(ivec3 b);
    ivec3 operator/(ivec3 b);

    ivec3 operator+(vec3 b);
    ivec3 operator-(vec3 b);
    ivec3 operator*(vec3 b);
    ivec3 operator/(vec3 b);

    ivec3 operator+(uvec3 b);
    ivec3 operator-(uvec3 b);
    ivec3 operator*(uvec3 b);
    ivec3 operator/(uvec3 b);
};

class uvec3 {
public:
    u32 x, y, z;
    uvec3(u32 x = 0, u32 y = 0, u32 z = 0) : x(x), y(y), z(z) {}

    uvec3 operator+(ivec3 b);
    uvec3 operator-(ivec3 b);
    uvec3 operator*(ivec3 b);
    uvec3 operator/(ivec3 b);

    uvec3 operator+(vec3 b);
    uvec3 operator-(vec3 b);
    uvec3 operator*(vec3 b);
    uvec3 operator/(vec3 b);

    uvec3 operator+(uvec3 b);
    uvec3 operator-(uvec3 b);
    uvec3 operator*(uvec3 b);
    uvec3 operator/(uvec3 b);
};

class ivec4 {
public:
    i32 x, y, z, w;
    ivec4(i32 x = 0, i32 y = 0, i32 z = 0, i32 w = 0) : x(x), y(y), z(z), w(w) {}
};

class uvec4 {
public:
    u32 x, y, z, w;
    uvec4(u32 x = 0, u32 y = 0, u32 z = 0, u32 w = 0) : x(x), y(y), z(z), w(w) {}
};