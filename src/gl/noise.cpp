#include "noise.hpp"

f64 Perlin::rand2(vec2 p, const u64 _seed) {
    const u64 seed = (_seed % 0x223a25a4) ^ (u64)(p.x + p.y);
    constexpr f64 BIG = 1.0f / (f64)mu_i_infinity_32;
    const u64 x = p.x, y = p.y;
    const u64 a = ~((x * (y ^ seed)) ^ 0xd5b67268) % ((seed == 0) ? 1 : seed),
              d = seed * a,
              b =  ((y * (x ^ seed)) ^ 0x6e51022c) % ((d == 0) ? 1 : d);
    const u64 g = a ^ b;
    const u64 r =  (((((g) * ~(seed) * a) % 0x880a3c0f) * seed) ^ g) & 0xffffffff;
                
    return ((((f64)r * BIG) * 2.0) + 1.0) * 0.5;
}

//TODO: this function
vec2 Perlin::rand3(vec3 p, const u64 _seed) {
    const u64 seed = (_seed % 0x223a25a4) ^ (u64)(p.x + p.y + p.z);
    constexpr f64 BIG = 1.0f / (f64)mu_i_infinity_32;
    const u64 x = p.x, y = p.y, z = p.z;
    const u64 a = ~((x * (y ^ seed)) ^ 0xd5b67268) % (seed + 1),
              b =  ((y * (z ^ seed)) ^ 0x6e51022c) % (seed * a + 1),
              c =  ((z * (x ^ seed)) ^ 0x5f3c0e64) % (seed ^ b);
    const u64 g = a ^ b;
    const u64 r0 = (((((g ^ a) * ~(seed) * c) % 0x880a3c0f) * seed) ^ a) & 0xffffffff,
              r1 = (((((g ^ b) *  (seed) * c) % 0xfb65c8bf) * ~seed) ^ b) & 0xffffffff;
                
    return {
        (f32) (((((f32)r0 * BIG) * 2.0f) + 1.0f) * 0.5f),
        (f32) (((((f32)r1 * BIG) * 2.0f) + 1.0f) * 0.5f)
    };
}

vec2 Perlin::GenVec2(vec2 pos) {
    const f32 theta = Perlin::rand2(pos, this->seed) * mu_pi * 2.0f;

    return {
        cosf(theta),
        sinf(theta)
    };
}

vec3 Perlin::GenVec3(vec3 pos) {
    const vec2 ang = Perlin::rand3(pos, this->seed) * mu_pi * 2.0f;

    return {
        cosf(ang.y) * cosf(ang.x),
        sinf(ang.y),
        cosf(ang.y) * sinf(ang.x)
    };
}

f32 Perlin::fade(f32 t) {
    return t * t * t * (t * (t * 6.0f - 15.0f) + 10.0f);
}

f32 Perlin::noise2d(vec2 pos) {
    const f32 ix = floorf(pos.x), iy = floorf(pos.y),
              xf = pos.x - ix, yf = pos.y - iy;

    const vec2 v00 = {xf - 1.0f, yf - 1.0f}, //tl
               v01 = {xf - 0.0f, yf - 1.0f}, //tr
               v10 = {xf - 1.0f, yf - 0.0f}, //bl
               v11 = {xf - 0.0f, yf - 0.0f}; //br

    const vec2 aa = Perlin::GenVec2({ix + 1.0f, iy + 1.0f}),
               ab = Perlin::GenVec2({ix + 0.0f, iy + 1.0f}),
               ba = Perlin::GenVec2({ix + 1.0f, iy + 0.0f}),
               bb = Perlin::GenVec2({ix + 0.0f, iy + 0.0f});

    const f32 d00 = vec2::DotProd(v00, aa),
              d01 = vec2::DotProd(v01, ab),
              d10 = vec2::DotProd(v10, ba),
              d11 = vec2::DotProd(v11, bb);

    const f32 m0 = Perlin::fade(xf), m1 = Perlin::fade(yf);

    const f32 i0 = lerp(d11, d01, m1),
              i1 = lerp(d10, d00, m1);

    return lerp(i0, i1, m0);
}

f32 Perlin::advNoise2d(vec2 pos, Perlin::NoiseSettings ns) {
    f32 s = 0.0f, g = 1.0f / ns.nLayers;
    f32 amp = ns.amp, freq = ns.freq;

    forrange(ns.nLayers) {
        s += this->noise2d(
            pos * freq
        ) * amp * g;

        amp *= ns.ampScale;
        freq *= ns.freqScale;
    }

    return s;
}

f32 Perlin::noise3d(vec3 pos) {
    return 0.0f;
}

f32 Perlin::advNoise3d(vec3 pos, Perlin::NoiseSettings ns) {
    f32 s = 0.0f, g = 1.0f / ns.nLayers;
    f32 amp = ns.amp, freq = ns.freq;

    forrange(ns.nLayers) {
        s += this->noise3d(
            pos * freq
        ) * amp * g;

        amp *= ns.ampScale;
        freq *= ns.freqScale;
    }

    return s;
}

f32 Perlin::noise4d(vec4 pos) {
    return 0.0f;
}

f32 Perlin::advNoise4d(vec4 pos, Perlin::NoiseSettings ns) {
    f32 s = 0.0f, g = 1.0f / ns.nLayers;
    f32 amp = ns.amp, freq = ns.freq;

    forrange(ns.nLayers) {
        s += this->noise4d(
            pos * freq
        ) * amp * g;

        amp *= ns.ampScale;
        freq *= ns.freqScale;
    }

    return s;
}