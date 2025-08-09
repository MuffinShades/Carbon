#pragma once
#include <iostream>
#include "../vec.hpp"
#include "../msutil.hpp"

class Perlin {
private:
    static f64 rand2(vec2 p, const u64 seed);
    static vec2 rand3(vec3 p, const u64 seed);

    vec2 GenVec2(vec2 pos);
    vec3 GenVec3(vec3 pos);

    static f32 fade(f32 t);

    u64 seed;
public:
    struct NoiseSettings {
        u32 nLayers = 1;
        f32 freq = 1.0f, amp = 1.0f;
        f32 ampScale = 1.0f, freqScale = 1.0f;
    };

    Perlin(const u64 seed) {
        this->seed = seed;
    }

    Perlin() {
        this->seed = 0; //TODO: random seed
    }

    f32 noise2d(vec2 pos);
    f32 advNoise2d(vec2 pos, NoiseSettings ns);

    f32 noise3d(vec3 pos);
    f32 advNoise3d(vec3 pos, NoiseSettings ns);

    f32 noise4d(vec4 pos);
    f32 advNoise4d(vec4 pos, NoiseSettings ns);
};

class Simplex {
private:
    u32 seed;
public:
    f32 noise2d(vec2 pos);
};