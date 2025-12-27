#pragma once
#include "ttf.hpp"
#include "bitmap.hpp"

/**
 * 
 * ttf_render.hpp
 * 
 * For rendering ttf glyphs
 * 
 * written by muffinshades 2024-2025
 * 
 * Stil working on this ;-;
 * 
 * 
 */

enum class sdf_dim_ty {
    Width,
    Height,
    Scale
};

struct sdf_dim {
    union {
        u32 w;
        u32 h;
        u32 scale;
    } m;
    sdf_dim_ty slc;
};

static sdf_dim sdf_width_dim(u32 w) {
    sdf_dim d;

    d.m.w = w;
    d.slc = sdf_dim_ty::Width;

    return d;
}

static sdf_dim sdf_height_dim(u32 h) {
    sdf_dim d;

    d.m.h = h;
    d.slc = sdf_dim_ty::Height;

    return d;
}

static sdf_dim sdf_scale_dim(u32 s) {
    sdf_dim d;

    d.m.scale = s;
    d.slc = sdf_dim_ty::Scale;

    return d;
}

#ifdef MSFL_DLL
#ifdef MSFL_EXPORTS
#define MSFL_EXP __declspec(dllexport)
#else
#define MSFL_EXP __declspec(dllimport)
#endif
#else
#define MSFL_EXP
#endif

#ifdef MSFL_DLL
#ifdef __cplusplus
extern "C" {
#endif
#endif

class ttfRender {
public:
    MSFL_EXP static i32 RenderGlyphToBitmap(Glyph tGlyph, Bitmap* bmp, float scale = 1.0f);
    MSFL_EXP static i32 RenderGlyphOutlineToBitmap(Glyph tGlyph, Bitmap* bmp, sdf_dim size);
    MSFL_EXP static i32 RenderGlyphSDFToBitMap(Glyph tGlyph, Bitmap* bmp, sdf_dim size);
    MSFL_EXP static i32 RenderGlyphPseudoSDFToBitMap(Glyph tGlyph, Bitmap* bmp, sdf_dim size);
    MSFL_EXP static i32 RenderGlyphMSDFToBitMap(Glyph tGlyph, Bitmap* bmp, sdf_dim size);
    //MSFL_EXP static i32 RenderGlyphMSFGToBitMap(Glyph tGlyph, Bitmap* bmp, size_t glyphW, size_t glyphH);
    MSFL_EXP static i32 RenderSDFToBitmap(Bitmap* sdf, Bitmap* bmp, sdf_dim res_size);
    MSFL_EXP static i32 RenderMSDFToBitmap(Bitmap* sdf, Bitmap* bmp, sdf_dim res_size);
};

#ifdef MSFL_DLL
#ifdef __cplusplus
}
#endif
#endif