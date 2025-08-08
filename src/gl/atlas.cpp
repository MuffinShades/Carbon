#include "atlas.hpp"
#include "../vec.hpp"

TexAtlas::TexAtlas(size_t imgW, size_t imgH, size_t cellW, size_t cellH) {
    this->iw = imgW;
    this->ih = imgH;
    this->cw = cellW;
    this->ch = cellH;
    this->invW = 1.0f / this->iw;
    this->invH = 1.0f / this->ih;
    this->ncw = cellW * this->invW;
    this->nch = cellH * this->invH;
}

vec2 TexAtlas::getIndexCoords(u32 idxX, u32 idxY) {
    return this->getNormalCoords(idxX * this->cw, idxY * this->ch);
}

vec2 TexAtlas::getNormalCoords(i32 x, i32 y) {
    return vec2(
        x * this->invW,
        y * this->invH
    );
}

TexPart TexAtlas::getImageIndexPart(u32 idxX, u32 idxY) {
    vec2 tl = this->getNormalCoords(idxX * this->cw, idxY * this->ch);

    return {
        .tl = tl,
        .tr = tl + vec2(ncw, 0.0f),
        .br = tl + vec2(ncw, nch),
        .bl = tl + vec2(0.0f, nch)
    };
}