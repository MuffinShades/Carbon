#pragma once
#include <iostream>
#include "../msutil.hpp"
#include "../vec.hpp"

struct TexPart {
    vec2 tl, tr, bl, br;
};

class TexAtlas {
private:
    f32 iw, ih, cw, ch;
    f64 invW, invH, ncw, nch;
public:
    TexAtlas(size_t imgW, size_t imgH, size_t cellW, size_t cellH);
    vec2 getIndexCoords(u32 idxX, u32 idxY);
    vec2 getNormalCoords(i32 x, i32 y);
    TexPart getImageIndexPart(u32 idxX, u32 idxY);
};