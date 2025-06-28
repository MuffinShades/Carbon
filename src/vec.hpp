#pragma once
#include <iostream>
#include <cmath>

class vec2 {
public:
    float x = 0.0f, y = 0.0f;
    vec2(float x = 0.0f, float y = 0.0f) : x(x), y(y) {}

    //operators
    vec2 operator+(vec2 b) {
        return vec2(this->x + b.x, this->y + b.y);
    }

    vec2 operator-(vec2 b) {
        return vec2(this->x - b.x, this->y - b.y);
    }

    vec2 operator*(vec2 b) {
        return vec2(this->x * b.x, this->y * b.y);
    }

    vec2 operator*(float s) {
        return vec2(this->x * s, this->y * s);
    }

    vec2 operator/(float s) {
        if (s == 0.0f)
            return *this;

        return vec2(this->x / s, this->y / s);
    }

    vec2 operator/(vec2 b) {
        if (b.x == 0.0f || b.y == 0.0f)
            b = vec2(1.0f, 1.0f);

        return vec2(this->x / b.x, this->y / b.y);
    }

    static float DotProd(vec2 a, vec2 b) {
        return a.x * b.x + a.y * b.y;
    }

    static float CrossProd(vec2 a, vec2 b) {
        return a.x * b.y - a.y * b.x;
    }

    float lenSqr() {
        return this->x*this->x + this->y*this->y;
    }

    float len() {
        return sqrt(this->lenSqr());
    }

    void Normalize() {
        const float length = this->len();
        this->x /= length;
        this->y /= length;
    }

    vec2 GetNormal() {
        const float length = this->len();

        return vec2(this->x / length, this->y / length);
    }
};

class vec3 {
public:
    float x = 0.0f, y = 0.0f, z = 0.0f;
    vec3(float x = 0.0f, float y = 0.0f, float z = 0.0f) : x(x), y(y), z(z) {}

    //operators
    vec3 operator+(vec3 b) {
        return vec3(this->x + b.x, this->y + b.y, this->z + b.z);
    }

    vec3 operator-(vec3 b) {
        return vec3(this->x - b.x, this->y - b.y, this->z - b.z);
    }

    vec3 operator*(vec3 b) {
        return vec3(this->x * b.x, this->y * b.y, this->z * b.z);
    }

    vec3 operator*(float s) {
        return vec3(this->x * s, this->y * s, this->z * s);
    }

    vec3 operator/(float s) {
        if (s == 0.0f)
            return *this;

        return vec3(this->x / s, this->y / s, this->z / s);
    }

    vec3 operator/(vec3 b) {
        if (b.x == 0.0f || b.y == 0.0f || b.z == 0.0f)
            b = vec3(1.0f, 1.0f);

        return vec3(this->x / b.x, this->y / b.y, this->z / b.z);
    }

    static float DotProd(vec3 a, vec3 b) {
        return a.x * b.x + a.y * b.y + a.z * b.z;
    }

    static vec3 CrossProd(vec3 a, vec3 b) {
        return vec3(
            a.y * b.z - a.z * b.y, 
            a.x * b.z - a.z * b.x,
            a.x * b.y - a.y * b.x
        );
    }

    float lenSqr() {
        return this->x * this->x + this->y * this->y + this->z * this->z;
    }

    float len() {
        return sqrt(this->lenSqr());
    }

    static vec3 Normalize(vec3 v) {
        const float length = v.len();
        v.x /= length;
        v.y /= length;
        v.z /= length;
    }

    vec3 GetNormal() {
        const float length = this->len();

        return vec3(this->x / length, this->y / length, this->z / length);
    }
};

class vec4 {
public:
    float x = 0.0f, y = 0.0f, z = 0.0f, w = 0.0f;
    vec4(float x = 0.0f, float y = 0.0f, float z = 0.0f, float w = 0.0f) : x(x), y(y), z(z), w(w) {}

    //operators
    vec4 operator+(vec4 b) {
        return vec4(this->x + b.x, this->y + b.y, this->z + b.z, this->w + b.w);
    }

    vec4 operator-(vec4 b) {
        return vec4(this->x - b.x, this->y - b.y, this->z - b.z, this->w - b.w);
    }

    vec4 operator*(vec4 b) {
        return vec4(this->x * b.x, this->y * b.y, this->z * b.z, this->w * b.w);
    }

    vec4 operator*(float s) {
        return vec4(this->x * s, this->y * s, this->z * s, this->w * s);
    }

    vec4 operator/(float s) {
        if (s == 0.0f)
            return *this;

        return vec4(this->x / s, this->y / s, this->z / s, this->w / s);
    }

    vec4 operator/(vec4 b) {
        if (b.x == 0.0f || b.y == 0.0f || b.z == 0.0f)
            b = vec4(1.0f, 1.0, 1.0f, 1.0f);

        return vec4(this->x / b.x, this->y / b.y, this->z / b.z, this->w / b.w);
    }

    static float DotProd(vec4 a, vec4 b) {
        return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
    }

    float lenSqr() {
        return this->x * this->x + this->y * this->y + this->z * this->z + this->w * this->w;
    }

    float len() {
        return sqrt(this->lenSqr());
    }

    static vec4 Normalize(vec4 v) {
        const float length = v.len();
        v.x /= length;
        v.y /= length;
        v.z /= length;
        v.w /= length;
    }

    vec4 GetNormal() {
        const float length = this->len();

        return vec4(this->x / length, this->y / length, this->z / length, this->w / length);
    }
};